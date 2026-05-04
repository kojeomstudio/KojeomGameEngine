#!/usr/bin/env bash
set -euo pipefail

PROMPT_INPUT="${prompt_content:-}"

echo "Starting Coding Agent CLI execution."
echo "Received prompt: $PROMPT_INPUT"

if [ -z "$PROMPT_INPUT" ]; then
    echo "Error: Environment variable is not set." >&2
    exit 1
fi

if ! kilo run "$PROMPT_INPUT" --auto; then
    EXIT_CODE=$?
    echo "Error: Code CLI execution failed. Exit Code: $EXIT_CODE" >&2
    exit "$EXIT_CODE"
fi

echo "Code CLI execution completed."
