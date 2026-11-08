#include "nagram/services/credentials_linux.h"

#include "nagram/services/secret_service_model.h"

#include <gio/gio.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>

namespace Nagram::SecretService {
namespace {

constexpr auto kCallTimeout = 25 * 1000;

// Seconds to answer a prompt; the window does not repaint while waiting.
constexpr auto kPromptTimeout = 120u;

struct ObjectDeleter {
	void operator()(GDBusConnection *value) const {
		g_object_unref(value);
	}
};

struct VariantDeleter {
	void operator()(GVariant *value) const {
		g_variant_unref(value);
	}
};

using Bus = std::unique_ptr<GDBusConnection, ObjectDeleter>;
using Variant = std::unique_ptr<GVariant, VariantDeleter>;
using Paths = std::vector<QByteArray>;

std::atomic<bool> ReadPromptDismissed = false;

CredentialError Map(GError *error) {
	auto result = CredentialError::Unavailable;
	if (const auto name = g_dbus_error_get_remote_error(error)) {
		result = ErrorFromName(QByteArray(name));
		g_free(name);
	}
	g_error_free(error);
	return result;
}

Variant Call(
		GDBusConnection *bus,
		const QByteArray &path,
		const char *interface,
		const char *method,
		GVariant *parameters,
		const char *replyType,
		CredentialError &error) {
	auto raw = (GError*)nullptr;
	const auto result = g_dbus_connection_call_sync(
		bus,
		kBusName,
		path.constData(),
		interface,
		method,
		parameters,
		G_VARIANT_TYPE(replyType),
		G_DBUS_CALL_FLAGS_NONE,
		kCallTimeout,
		nullptr,
		&raw);
	if (!result) {
		error = raw ? Map(raw) : CredentialError::Unavailable;
		return nullptr;
	}
	return Variant(result);
}

Bus Connect(CredentialError &error) {
	auto raw = (GError*)nullptr;
	const auto bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &raw);
	if (!bus) {
		if (raw) {
			g_error_free(raw);
		}
		error = CredentialError::Unavailable;
		return nullptr;
	}
	return Bus(bus);
}

Paths ReadPaths(GVariant *array) {
	auto result = Paths();
	auto iter = GVariantIter();
	g_variant_iter_init(&iter, array);
	auto path = (const gchar*)nullptr;
	while (g_variant_iter_next(&iter, "&o", &path)) {
		result.emplace_back(path);
	}
	return result;
}

GVariant *PathsVariant(const Paths &paths) {
	auto builder = GVariantBuilder();
	g_variant_builder_init(&builder, G_VARIANT_TYPE("ao"));
	for (const auto &path : paths) {
		g_variant_builder_add(&builder, "o", path.constData());
	}
	return g_variant_builder_end(&builder);
}

GVariant *AttributesVariant(const QString &account) {
	auto builder = GVariantBuilder();
	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{ss}"));
	for (const auto &[name, value] : ItemAttributes(account)) {
		g_variant_builder_add(
			&builder,
			"{ss}",
			name.constData(),
			value.constData());
	}
	return g_variant_builder_end(&builder);
}

struct PromptState {
	bool completed = false;
	bool dismissed = false;
	bool timedOut = false;
	Variant result;
};

void PromptCompleted(
		GDBusConnection *connection,
		const gchar *sender,
		const gchar *path,
		const gchar *interface,
		const gchar *signal,
		GVariant *parameters,
		gpointer data) {
	const auto state = static_cast<std::shared_ptr<PromptState>*>(data)->get();
	if (state->completed
		|| !g_variant_is_of_type(parameters, G_VARIANT_TYPE("(bv)"))) {
		return;
	}
	auto dismissed = gboolean(FALSE);
	auto result = (GVariant*)nullptr;
	g_variant_get(parameters, "(bv)", &dismissed, &result);
	state->completed = true;
	state->dismissed = dismissed;
	state->result = Variant(result);
}

void PromptReleased(gpointer data) {
	delete static_cast<std::shared_ptr<PromptState>*>(data);
}

gboolean PromptTimedOut(gpointer data) {
	static_cast<PromptState*>(data)->timedOut = true;
	return G_SOURCE_REMOVE;
}

CredentialError RunPrompt(
		GDBusConnection *bus,
		const QByteArray &path,
		Variant *result) {
	const auto context = g_main_context_new();
	g_main_context_push_thread_default(context);
	const auto state = std::make_shared<PromptState>();
	const auto subscription = g_dbus_connection_signal_subscribe(
		bus,
		nullptr,
		kPromptInterface,
		"Completed",
		path.constData(),
		nullptr,
		G_DBUS_SIGNAL_FLAGS_NONE,
		PromptCompleted,
		new std::shared_ptr<PromptState>(state),
		PromptReleased);

	auto error = CredentialError::None;
	const auto shown = Call(
		bus,
		path,
		kPromptInterface,
		"Prompt",
		g_variant_new("(s)", ""),
		"()",
		error);
	if (shown) {
		const auto timeout = g_timeout_source_new_seconds(kPromptTimeout);
		g_source_set_callback(timeout, PromptTimedOut, state.get(), nullptr);
		g_source_attach(timeout, context);
		while (!state->completed && !state->timedOut) {
			g_main_context_iteration(context, TRUE);
		}
		g_source_destroy(timeout);
		g_source_unref(timeout);
		if (!state->completed) {
			auto ignored = CredentialError::None;
			Call(bus, path, kPromptInterface, "Dismiss", nullptr, "()", ignored);
			error = CredentialError::Denied;
		} else if (state->dismissed) {
			error = CredentialError::Denied;
		} else if (result) {
			*result = std::move(state->result);
		}
	}

	g_dbus_connection_signal_unsubscribe(bus, subscription);
	while (g_main_context_iteration(context, FALSE)) {
	}
	g_main_context_pop_thread_default(context);
	g_main_context_unref(context);
	return error;
}

CredentialError Search(
		GDBusConnection *bus,
		const QString &account,
		Paths &unlocked,
		Paths &locked) {
	auto error = CredentialError::None;
	const auto reply = Call(
		bus,
		kServicePath,
		kServiceInterface,
		"SearchItems",
		g_variant_new("(@a{ss})", AttributesVariant(account)),
		"(aoao)",
		error);
	if (!reply) {
		return error;
	}
	auto first = (GVariant*)nullptr;
	auto second = (GVariant*)nullptr;
	g_variant_get(reply.get(), "(@ao@ao)", &first, &second);
	const auto unlockedValue = Variant(first);
	const auto lockedValue = Variant(second);
	unlocked = ReadPaths(unlockedValue.get());
	locked = ReadPaths(lockedValue.get());
	return CredentialError::None;
}

CredentialError Unlock(
		GDBusConnection *bus,
		const Paths &objects,
		Paths &unlocked) {
	auto error = CredentialError::None;
	const auto reply = Call(
		bus,
		kServicePath,
		kServiceInterface,
		"Unlock",
		g_variant_new("(@ao)", PathsVariant(objects)),
		"(aoo)",
		error);
	if (!reply) {
		return error;
	}
	auto list = (GVariant*)nullptr;
	auto prompt = (const gchar*)nullptr;
	g_variant_get(reply.get(), "(@ao&o)", &list, &prompt);
	const auto listValue = Variant(list);
	unlocked = ReadPaths(listValue.get());
	if (NoObject(QByteArray(prompt))) {
		return CredentialError::None;
	}
	auto result = Variant();
	error = RunPrompt(bus, QByteArray(prompt), &result);
	if (error != CredentialError::None) {
		return error;
	}
	if (result && g_variant_is_of_type(result.get(), G_VARIANT_TYPE("ao"))) {
		for (auto &path : ReadPaths(result.get())) {
			unlocked.push_back(std::move(path));
		}
	}
	return CredentialError::None;
}

// Plain session: the provider runs as the same user, who can read the keyring.
class Session final {
public:
	Session(GDBusConnection *bus, CredentialError &error);
	Session(const Session &) = delete;
	Session &operator=(const Session &) = delete;
	~Session();

	[[nodiscard]] const QByteArray &path() const;

private:
	GDBusConnection *_bus = nullptr;
	QByteArray _path;

};

Session::Session(GDBusConnection *bus, CredentialError &error) : _bus(bus) {
	const auto reply = Call(
		bus,
		kServicePath,
		kServiceInterface,
		"OpenSession",
		g_variant_new("(sv)", "plain", g_variant_new_string("")),
		"(vo)",
		error);
	if (reply) {
		auto output = (GVariant*)nullptr;
		auto path = (const gchar*)nullptr;
		g_variant_get(reply.get(), "(v&o)", &output, &path);
		_path = QByteArray(path);
		g_variant_unref(output);
	}
}

Session::~Session() {
	if (!_path.isEmpty()) {
		auto ignored = CredentialError::None;
		Call(_bus, _path, kSessionInterface, "Close", nullptr, "()", ignored);
	}
}

const QByteArray &Session::path() const {
	return _path;
}

CredentialError DefaultCollection(GDBusConnection *bus, QByteArray &collection) {
	auto error = CredentialError::None;
	const auto alias = Call(
		bus,
		kServicePath,
		kServiceInterface,
		"ReadAlias",
		g_variant_new("(s)", "default"),
		"(o)",
		error);
	if (!alias) {
		return error;
	}
	auto path = (const gchar*)nullptr;
	g_variant_get(alias.get(), "(&o)", &path);
	collection = QByteArray(path);
	if (!NoObject(collection)) {
		return CredentialError::None;
	}

	auto properties = GVariantBuilder();
	g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(
		&properties,
		"{sv}",
		"org.freedesktop.Secret.Collection.Label",
		g_variant_new_string("Default keyring"));
	const auto created = Call(
		bus,
		kServicePath,
		kServiceInterface,
		"CreateCollection",
		g_variant_new(
			"(@a{sv}s)",
			g_variant_builder_end(&properties),
			"default"),
		"(oo)",
		error);
	if (!created) {
		return error;
	}
	auto prompt = (const gchar*)nullptr;
	g_variant_get(created.get(), "(&o&o)", &path, &prompt);
	collection = QByteArray(path);
	if (!NoObject(collection)) {
		return CredentialError::None;
	} else if (NoObject(QByteArray(prompt))) {
		return CredentialError::Denied;
	}
	auto result = Variant();
	error = RunPrompt(bus, QByteArray(prompt), &result);
	if (error != CredentialError::None) {
		return error;
	} else if (!result
		|| !g_variant_is_of_type(result.get(), G_VARIANT_TYPE_OBJECT_PATH)) {
		return CredentialError::Denied;
	}
	collection = QByteArray(g_variant_get_string(result.get(), nullptr));
	return NoObject(collection)
		? CredentialError::Denied
		: CredentialError::None;
}

} // namespace

CredentialResult Read(const QString &account) {
	auto error = CredentialError::None;
	const auto bus = Connect(error);
	if (!bus) {
		return { .error = error };
	}
	auto unlocked = Paths();
	auto locked = Paths();
	error = Search(bus.get(), account, unlocked, locked);
	if (error != CredentialError::None) {
		return { .error = error };
	}
	if (unlocked.empty()) {
		if (locked.empty()) {
			return { .error = CredentialError::Missing };
		} else if (ReadPromptDismissed) {
			return { .error = CredentialError::Denied };
		}
		error = Unlock(bus.get(), { locked.front() }, unlocked);
		if (error == CredentialError::Denied) {
			ReadPromptDismissed = true;
		}
		if (error != CredentialError::None) {
			return { .error = error };
		} else if (unlocked.empty()) {
			return { .error = CredentialError::Denied };
		}
	}

	const auto session = Session(bus.get(), error);
	if (session.path().isEmpty()) {
		return { .error = (error != CredentialError::None)
			? error
			: CredentialError::Unavailable };
	}
	const auto reply = Call(
		bus.get(),
		unlocked.front(),
		kItemInterface,
		"GetSecret",
		g_variant_new("(o)", session.path().constData()),
		"((oayays))",
		error);
	if (!reply) {
		return { .error = error };
	}
	auto value = (GVariant*)nullptr;
	g_variant_get(reply.get(), "((&o@ay@ay&s))", nullptr, nullptr, &value, nullptr);
	const auto bytes = Variant(value);
	auto size = gsize(0);
	const auto data = static_cast<const char*>(
		g_variant_get_fixed_array(bytes.get(), &size, 1));
	return { .secret = QByteArray(data, qsizetype(size)) };
}

CredentialError Write(const QString &account, const QByteArray &secret) {
	ReadPromptDismissed = false;
	auto error = CredentialError::None;
	const auto bus = Connect(error);
	if (!bus) {
		return error;
	}
	auto collection = QByteArray();
	error = DefaultCollection(bus.get(), collection);
	if (error != CredentialError::None) {
		return error;
	}
	auto unlocked = Paths();
	error = Unlock(bus.get(), { collection }, unlocked);
	if (error != CredentialError::None) {
		return error;
	} else if (std::find(begin(unlocked), end(unlocked), collection)
		== end(unlocked)) {
		return CredentialError::Denied;
	}

	const auto session = Session(bus.get(), error);
	if (session.path().isEmpty()) {
		return (error != CredentialError::None)
			? error
			: CredentialError::Unavailable;
	}
	auto properties = GVariantBuilder();
	g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(
		&properties,
		"{sv}",
		"org.freedesktop.Secret.Item.Label",
		g_variant_new_string(ItemLabel(account).constData()));
	g_variant_builder_add(
		&properties,
		"{sv}",
		"org.freedesktop.Secret.Item.Attributes",
		AttributesVariant(account));
	const auto created = Call(
		bus.get(),
		collection,
		kCollectionInterface,
		"CreateItem",
		g_variant_new(
			"(@a{sv}(o@ay@ays)b)",
			g_variant_builder_end(&properties),
			session.path().constData(),
			g_variant_new_array(G_VARIANT_TYPE_BYTE, nullptr, 0),
			g_variant_new_fixed_array(
				G_VARIANT_TYPE_BYTE,
				secret.constData(),
				gsize(secret.size()),
				1),
			"text/plain",
			TRUE),
		"(oo)",
		error);
	if (!created) {
		return error;
	}
	auto item = (const gchar*)nullptr;
	auto prompt = (const gchar*)nullptr;
	g_variant_get(created.get(), "(&o&o)", &item, &prompt);
	if (!NoObject(QByteArray(item))) {
		return CredentialError::None;
	} else if (NoObject(QByteArray(prompt))) {
		return CredentialError::Denied;
	}
	return RunPrompt(bus.get(), QByteArray(prompt), nullptr);
}

CredentialError Delete(const QString &account) {
	auto error = CredentialError::None;
	const auto bus = Connect(error);
	if (!bus) {
		return error;
	}
	auto items = Paths();
	auto locked = Paths();
	error = Search(bus.get(), account, items, locked);
	if (error != CredentialError::None) {
		return error;
	}
	if (!locked.empty()) {
		auto unlocked = Paths();
		error = Unlock(bus.get(), locked, unlocked);
		if (error != CredentialError::None) {
			return error;
		} else if (unlocked.size() < locked.size()) {
			return CredentialError::Denied;
		}
		items.insert(end(items), begin(unlocked), end(unlocked));
	}
	for (const auto &item : items) {
		const auto reply = Call(
			bus.get(),
			item,
			kItemInterface,
			"Delete",
			nullptr,
			"(o)",
			error);
		if (!reply) {
			if (error == CredentialError::Missing) {
				continue;
			}
			return error;
		}
		auto prompt = (const gchar*)nullptr;
		g_variant_get(reply.get(), "(&o)", &prompt);
		if (!NoObject(QByteArray(prompt))) {
			error = RunPrompt(bus.get(), QByteArray(prompt), nullptr);
			if (error != CredentialError::None) {
				return error;
			}
		}
	}
	return CredentialError::None;
}

} // namespace Nagram::SecretService
