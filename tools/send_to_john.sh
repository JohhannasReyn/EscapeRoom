#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=tools/lib-room.sh
source "${SCRIPT_DIR}/lib-room.sh"

CONFIG_FILE="${JOHN_CONTACT_ENV:-${PROJECT_ROOT}/john-contact.env}"
SEARCH_DIR="${FIRE_PANEL_LOG_DIR:-${PROJECT_ROOT}}"
LOG_FILE="${1:-}"

if [ -f "${CONFIG_FILE}" ]; then
    # shellcheck source=/dev/null
    source "${CONFIG_FILE}"
fi

JOHN_UPLOAD_URL="${JOHN_UPLOAD_URL:-}"
JOHN_RCLONE_TARGET="${JOHN_RCLONE_TARGET:-}"
JOHN_RCLONE_ROOT_FOLDER_ID="${JOHN_RCLONE_ROOT_FOLDER_ID:-}"
JOHN_SCP_TARGET="${JOHN_SCP_TARGET:-}"
JOHN_EMAIL="${JOHN_EMAIL:-}"
JOHN_EMAIL_SUBJECT="${JOHN_EMAIL_SUBJECT:-Escape room fire-panel button order log}"

save_email_config() {
    local email="$1"

    mkdir -p "$(dirname "${CONFIG_FILE}")"
    umask 077
    cat >"${CONFIG_FILE}" <<EOF
# Local send settings for tools/send_to_john.sh.
# This file is intentionally git-ignored.
JOHN_RCLONE_TARGET="${JOHN_RCLONE_TARGET}"
JOHN_RCLONE_ROOT_FOLDER_ID="${JOHN_RCLONE_ROOT_FOLDER_ID}"
JOHN_UPLOAD_URL="${JOHN_UPLOAD_URL}"
JOHN_SCP_TARGET="${JOHN_SCP_TARGET}"
JOHN_EMAIL="${email}"
JOHN_EMAIL_SUBJECT="${JOHN_EMAIL_SUBJECT}"
EOF

    echo "Saved email for future runs:"
    echo "  ${CONFIG_FILE}"
}

prompt_for_email_if_needed() {
    if [ -n "${JOHN_RCLONE_TARGET}" ] || [ -n "${JOHN_UPLOAD_URL}" ] || [ -n "${JOHN_SCP_TARGET}" ] || [ -n "${JOHN_EMAIL}" ]; then
        return
    fi

    if [ ! -t 0 ]; then
        return
    fi

    echo "No saved send destination was found."
    echo "You can enter John's email now and this script will save it for next time."
    read -r -p "Email address, or press Enter to skip: " entered_email

    if [ -z "${entered_email}" ]; then
        return
    fi

    if [[ "${entered_email}" != *@*.* ]]; then
        echo "That does not look like an email address, so I will not save it."
        return
    fi

    JOHN_EMAIL="${entered_email}"
    save_email_config "${JOHN_EMAIL}"
}

find_latest_log() {
    local latest=""
    local file

    shopt -s nullglob
    for file in "${SEARCH_DIR}"/fire-panel-button-order-*.txt "${PROJECT_ROOT}"/fire-panel-button-order-*.txt; do
        if [ -z "${latest}" ] || [ "${file}" -nt "${latest}" ]; then
            latest="${file}"
        fi
    done
    shopt -u nullglob

    printf '%s\n' "${latest}"
}

if [ -z "${LOG_FILE}" ]; then
    LOG_FILE="$(find_latest_log)"
fi

if [ -z "${LOG_FILE}" ] || [ ! -f "${LOG_FILE}" ]; then
    echo "No fire-panel button-order log found."
    echo "Create one first:"
    echo "  cd escape-room"
    echo "  tools/capture-fire-panel-buttons.sh"
    exit 1
fi

prompt_for_email_if_needed

echo "Preparing to send:"
echo "  ${LOG_FILE}"

send_by_rclone() {
    need_cmd rclone || return 1
    echo "Uploading log with JOHN_RCLONE_TARGET..."

    if [ -n "${JOHN_RCLONE_ROOT_FOLDER_ID}" ]; then
        if ! RCLONE_DRIVE_ROOT_FOLDER_ID="${JOHN_RCLONE_ROOT_FOLDER_ID}" rclone copy "${LOG_FILE}" "${JOHN_RCLONE_TARGET}"; then
            echo "Drive upload failed."
            return 1
        fi
    else
        if ! rclone copy "${LOG_FILE}" "${JOHN_RCLONE_TARGET}"; then
            echo "Drive upload failed."
            return 1
        fi
    fi

    echo "Drive upload complete."
}

send_by_upload() {
    need_cmd curl || return 1
    echo "Uploading log with JOHN_UPLOAD_URL..."
    if ! curl -fsS -F "file=@${LOG_FILE}" "${JOHN_UPLOAD_URL}"; then
        echo "HTTP upload failed."
        return 1
    fi
    echo
    echo "Upload complete."
}

send_by_scp() {
    need_cmd scp || return 1
    echo "Copying log with JOHN_SCP_TARGET..."
    if ! scp "${LOG_FILE}" "${JOHN_SCP_TARGET}"; then
        echo "SCP copy failed."
        return 1
    fi
    echo "SCP copy complete."
}

send_by_email() {
    local body
    body="Attached is the escape room fire-panel button order log from $(hostname)."

    if command -v mutt >/dev/null 2>&1; then
        if printf '%s\n' "${body}" | mutt -s "${JOHN_EMAIL_SUBJECT}" -a "${LOG_FILE}" -- "${JOHN_EMAIL}"; then
            echo "Email sent with mutt."
            return
        fi
    fi

    if command -v mailx >/dev/null 2>&1; then
        if printf '%s\n' "${body}" | mailx -s "${JOHN_EMAIL_SUBJECT}" -a "${LOG_FILE}" "${JOHN_EMAIL}"; then
            echo "Email sent with mailx."
            return
        fi
    fi

    if command -v mail >/dev/null 2>&1; then
        if printf '%s\n' "${body}" | mail -s "${JOHN_EMAIL_SUBJECT}" -A "${LOG_FILE}" "${JOHN_EMAIL}" 2>/dev/null; then
            echo "Email sent with mail."
            return
        fi
    fi

    return 1
}

bundle_for_manual_send() {
    need_cmd tar
    local bundle="${LOG_FILE%.*}-for-john.tar.gz"
    tar -czf "${bundle}" -C "$(dirname "${LOG_FILE}")" "$(basename "${LOG_FILE}")"

    echo
    echo "No automatic send destination is configured yet."
    echo "Created this bundle for John:"
    echo "  ${bundle}"
    echo
    echo "To automate next time, copy john-contact.env.example to john-contact.env"
    echo "and set JOHN_RCLONE_TARGET, JOHN_UPLOAD_URL, JOHN_SCP_TARGET, or JOHN_EMAIL."
    echo "For the shared Google Drive folder, run tools/setup-drive-upload.sh first."
    if [ -n "${JOHN_EMAIL}" ]; then
        echo "Saved email: ${JOHN_EMAIL}"
        echo "Automatic email still needs mutt, mailx, or mail configured on the Pi."
    fi
}

if [ -n "${JOHN_RCLONE_TARGET}" ] && send_by_rclone; then
    :
elif [ -n "${JOHN_UPLOAD_URL}" ] && send_by_upload; then
    :
elif [ -n "${JOHN_SCP_TARGET}" ] && send_by_scp; then
    :
elif [ -n "${JOHN_EMAIL}" ] && send_by_email; then
    :
else
    bundle_for_manual_send
fi
