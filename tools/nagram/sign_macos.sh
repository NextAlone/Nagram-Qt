#!/usr/bin/env bash
# Sign a built Nagram Desktop.app, or the disk image that holds it, for distribution
# outside the App Store.
#
# Usage: sign_macos.sh <Nagram Desktop.app> <entitlements>
#        sign_macos.sh <Nagram.dmg>
# With the certificate and one set of notary credentials the bundle is signed
# with the Developer ID certificate under the hardened runtime, notarized and
# stapled; the image is signed, notarized and stapled the same way. With none
# of the variables the bundle is signed ad hoc, which Gatekeeper refuses until
# the user allows the app, and the image is left as it is. A partial set is an
# error.
#   NAGRAM_MACOS_CERTIFICATE           base64 of the Developer ID Application .p12
#   NAGRAM_MACOS_CERTIFICATE_PASSWORD  the password of that .p12
# Notary credentials, either an App Store Connect team API key:
#   NAGRAM_NOTARY_KEY                  the key, as the .p8 text
#   NAGRAM_NOTARY_KEY_ID               the id of that key
#   NAGRAM_NOTARY_ISSUER_ID            the issuer id of that key
# or the Apple ID of a team member:
#   NAGRAM_NOTARY_APPLE_ID             the Apple ID
#   NAGRAM_NOTARY_PASSWORD             an app-specific password of that Apple ID
#   NAGRAM_NOTARY_TEAM_ID              the id of the team that owns the certificate
set -euo pipefail

app=""
image=""
if [ $# -eq 1 ] && [ -f "$1" ] && [ "${1%.dmg}" != "$1" ]; then
	image=$1
elif [ $# -eq 2 ] && [ -d "$1/Contents/MacOS" ] && [ -f "$2" ]; then
	app=$1
	entitlements=$2
else
	echo "Usage: $0 <Nagram Desktop.app> <entitlements>" >&2
	echo "       $0 <Nagram.dmg>" >&2
	exit 2
fi

certificate=(NAGRAM_MACOS_CERTIFICATE NAGRAM_MACOS_CERTIFICATE_PASSWORD)
api_key=(NAGRAM_NOTARY_KEY NAGRAM_NOTARY_KEY_ID NAGRAM_NOTARY_ISSUER_ID)
apple_id=(NAGRAM_NOTARY_APPLE_ID NAGRAM_NOTARY_PASSWORD NAGRAM_NOTARY_TEAM_ID)

# Prints those of the named variables that are empty.
unset_of() {
	local name
	for name in "$@"; do
		[ -n "${!name:-}" ] || printf '%s ' "$name"
	done
}
no_certificate=$(unset_of "${certificate[@]}")
no_api_key=$(unset_of "${api_key[@]}")
no_apple_id=$(unset_of "${apple_id[@]}")
every="${certificate[*]} ${api_key[*]} ${apple_id[*]} "
if [ "$no_certificate$no_api_key$no_apple_id" = "$every" ]; then
	if [ -n "$image" ]; then
		echo "::warning::No Developer ID secrets: the disk image is not signed and not notarized."
	else
		echo "::warning::No Developer ID secrets: Nagram Desktop.app is signed ad hoc and not notarized."
		codesign --force --deep --sign - "$app"
	fi
	exit 0
elif [ -n "$no_certificate" ]; then
	echo "Developer ID signing needs ${certificate[*]}; missing: $no_certificate" >&2
	exit 1
elif [ -n "$no_api_key" ] && [ -n "$no_apple_id" ]; then
	echo "Notarization needs all of ${api_key[*]} (missing: $no_api_key)" \
		"or all of ${apple_id[*]} (missing: $no_apple_id)" >&2
	exit 1
fi

work=$(mktemp -d)
keychain="$work/nagram-signing.keychain-db"
keychains=()
while IFS= read -r line; do
	line=${line#*\"}
	keychains+=("${line%\"*}")
done < <(security list-keychains -d user)

cleanup() {
	security list-keychains -d user -s ${keychains[@]+"${keychains[@]}"} > /dev/null || true
	security delete-keychain "$keychain" 2> /dev/null || true
	rm -rf "$work"
}
trap cleanup EXIT

password=$(od -An -N24 -tx1 /dev/urandom | tr -d ' \n')
security create-keychain -p "$password" "$keychain"
security set-keychain-settings -lut 21600 "$keychain"
security unlock-keychain -p "$password" "$keychain"
printf '%s' "$NAGRAM_MACOS_CERTIFICATE" | base64 --decode > "$work/certificate.p12"
security import "$work/certificate.p12" -f pkcs12 -k "$keychain" \
	-P "$NAGRAM_MACOS_CERTIFICATE_PASSWORD" -T /usr/bin/codesign > /dev/null
security set-key-partition-list -S apple-tool:,apple:,codesign: -s \
	-k "$password" "$keychain" > /dev/null
security list-keychains -d user -s "$keychain" ${keychains[@]+"${keychains[@]}"}

identity=$(security find-identity -v -p codesigning "$keychain" \
	| sed -n 's/^ *[0-9]*) \([0-9A-F]\{40\}\) "Developer ID Application: .*/\1/p' \
	| head -n 1)
if [ -z "$identity" ]; then
	echo "The certificate holds no 'Developer ID Application' identity." >&2
	exit 1
fi

if [ -z "$no_api_key" ]; then
	printf '%s\n' "$NAGRAM_NOTARY_KEY" > "$work/notary.p8"
	notary=(--key "$work/notary.p8" --key-id "$NAGRAM_NOTARY_KEY_ID"
		--issuer "$NAGRAM_NOTARY_ISSUER_ID")
else
	notary=(--apple-id "$NAGRAM_NOTARY_APPLE_ID" --password "$NAGRAM_NOTARY_PASSWORD"
		--team-id "$NAGRAM_NOTARY_TEAM_ID")
fi

sign() {
	codesign --force --timestamp --options runtime \
		--keychain "$keychain" --sign "$identity" "$@"
}

# Submits the file to the notary service and waits for its verdict.
notarize() {
	xcrun notarytool submit "$1" "${notary[@]}" \
		--wait --timeout 45m --output-format json > "$work/notary.json"
	read -r id status < <(python3 -c '
import json, sys
result = json.load(open(sys.argv[1]))
print(result.get("id", "-"), result.get("status", "-"))
' "$work/notary.json")
	if [ "$status" != "Accepted" ]; then
		echo "Notarization ended with the status $status." >&2
		xcrun notarytool log "$id" "${notary[@]}" >&2 || true
		exit 1
	fi
}

if [ -n "$image" ]; then
	# An image holds no code of its own for the hardened runtime.
	codesign --force --timestamp --keychain "$keychain" --sign "$identity" "$image"
	codesign --verify --verbose=2 "$image"
	notarize "$image"
	xcrun stapler staple "$image"
	xcrun stapler validate "$image"
	spctl --assess --type open --context context:primary-signature --verbose=2 "$image"
else
	# Nested code is signed before the bundle that seals it.
	if [ -d "$app/Contents/Frameworks" ]; then
		while IFS= read -r nested; do
			sign "$nested"
		done < <(find "$app/Contents/Frameworks" -type f \
			\( -name '*.dylib' -o -perm -100 \) | sort -r)
	fi
	sign --entitlements "$entitlements" "$app"
	codesign --verify --deep --strict --verbose=2 "$app"

	ditto -c -k --keepParent "$app" "$work/notarize.zip"
	notarize "$work/notarize.zip"
	xcrun stapler staple "$app"
	xcrun stapler validate "$app"
	spctl --assess --type execute --verbose=2 "$app"
fi
echo "Signed with the Developer ID identity $identity, notarized ($id) and stapled."
