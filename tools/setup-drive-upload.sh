#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

CONFIG_FILE="${JOHN_CONTACT_ENV:-${PROJECT_ROOT}/john-contact.env}"
DEFAULT_REMOTE="escape-room-drive"
DEFAULT_FOLDER_ID="1QYtv2RmXZq8g6iEgjdxn2W2cG39L_TYW"
DRIVE_URL="https://drive.google.com/drive/folders/${DEFAULT_FOLDER_ID}?usp=sharing"

if [ -f "${CONFIG_FILE}" ]; then
    # shellcheck source=/dev/null
    source "${CONFIG_FILE}"
fi

JOHN_RCLONE_TARGET="${JOHN_RCLONE_TARGET:-${DEFAULT_REMOTE}:}"
JOHN_RCLONE_ROOT_FOLDER_ID="${JOHN_RCLONE_ROOT_FOLDER_ID:-${DEFAULT_FOLDER_ID}}"
JOHN_UPLOAD_URL="${JOHN_UPLOAD_URL:-}"
JOHN_SCP_TARGET="${JOHN_SCP_TARGET:-}"
JOHN_EMAIL="${JOHN_EMAIL:-}"
JOHN_EMAIL_SUBJECT="${JOHN_EMAIL_SUBJECT:-Escape room fire-panel button order log}"

mkdir -p "$(dirname "${CONFIG_FILE}")"
umask 077
cat >"${CONFIG_FILE}" <<EOF
# Local send settings for tools/send_to_john.sh.
# This file is intentionally git-ignored.
JOHN_RCLONE_TARGET="${JOHN_RCLONE_TARGET}"
JOHN_RCLONE_ROOT_FOLDER_ID="${JOHN_RCLONE_ROOT_FOLDER_ID}"
JOHN_UPLOAD_URL="${JOHN_UPLOAD_URL}"
JOHN_SCP_TARGET="${JOHN_SCP_TARGET}"
JOHN_EMAIL="${JOHN_EMAIL}"
JOHN_EMAIL_SUBJECT="${JOHN_EMAIL_SUBJECT}"
EOF

echo "Saved shared Drive upload settings:"
echo "  ${CONFIG_FILE}"
echo "Drive folder:"
echo "  ${DRIVE_URL}"
echo

if ! command -v rclone >/dev/null 2>&1; then
    cat <<EOF
rclone is not installed yet.

Install it on the Raspberry Pi, then run this setup script again:
  sudo apt update
  sudo apt install -y rclone
  cd escape-room
  tools/setup-drive-upload.sh
EOF
    exit 0
fi

remote_name="${JOHN_RCLONE_TARGET%%:*}"

if rclone listremotes 2>/dev/null | grep -qx "${remote_name}:"; then
    cat <<EOF
rclone remote "${remote_name}" exists.

To test Drive access:
  RCLONE_DRIVE_ROOT_FOLDER_ID="${JOHN_RCLONE_ROOT_FOLDER_ID}" rclone lsd "${JOHN_RCLONE_TARGET}"

To send the newest fire-panel log:
  tools/send_to_john.sh
EOF
    exit 0
fi

cat <<EOF
rclone is installed, but the "${remote_name}" remote is not configured yet.

Run:
  rclone config

Use these choices:
  n                        New remote
  ${remote_name}           Remote name
  drive                    Storage type
  drive                    Scope, or full Drive access if shown as a number
  blank                    Client ID and client secret
  blank                    Service account file
  n                        Browser auth if you are SSH'd into the Pi
  n                        Shared Drive, unless this folder is in a Google Workspace shared drive

If rclone gives an authorize command for another computer, run that command on
the student's Mac, sign in to the Google account that can access this folder,
then paste the token back into the Pi.

After rclone is configured, test it with:
  RCLONE_DRIVE_ROOT_FOLDER_ID="${JOHN_RCLONE_ROOT_FOLDER_ID}" rclone lsd "${JOHN_RCLONE_TARGET}"

Then send the newest fire-panel log with:
  tools/send_to_john.sh
EOF
