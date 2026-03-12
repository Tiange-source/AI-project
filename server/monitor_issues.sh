#!/bin/bash

# GitHub Issue Monitor Script
# Runs every 10 minutes to check for new issues/comments

export GH_TOKEN="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"
REPO="Tiange-source/AI-project"
LOG_FILE="/root/ai/server/monitor.log"

echo "=========================================="
echo "GitHub Issue Monitor"
echo "Time: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=========================================="
echo ""

# Check if server is running
if ! ps aux | grep gomoku_server | grep -v grep > /dev/null; then
    echo "⚠️  WARNING: Server is NOT running!"
    echo "Restarting server..."
    cd /root/ai/server
    nohup ./build/bin/gomoku_server -c config/server.conf > logs/gomoku_server.log 2>&1 &
    sleep 3
    if ps aux | grep gomoku_server | grep -v grep > /dev/null; then
        echo "✅ Server restarted successfully"
    else
        echo "❌ Failed to restart server"
    fi
else
    echo "✅ Server is running"
fi
echo ""

# Check issue #11 specifically (main focus)
echo "Checking issue #11 details..."
ISSUE_11_COMMENTS=$(curl -s -H "Authorization: token $GH_TOKEN" https://api.github.com/repos/$REPO/issues/11/comments 2>/dev/null | python3 -c "import json, sys; data = json.load(sys.stdin); print(len(data))" 2>/dev/null || echo "0")

echo "Issue #11 has $ISSUE_11_COMMENTS comments"

if [ "$ISSUE_11_COMMENTS" -gt 3 ]; then
    echo "⚠️  New comments detected in issue #11!"
    echo "Latest comment:"
    curl -s -H "Authorization: token $GH_TOKEN" https://api.github.com/repos/$REPO/issues/11/comments 2>/dev/null | python3 -c "
import json, sys
data = json.load(sys.stdin)
if data:
    latest = data[-1]
    print('---')
    print(f'User: {latest[\"user\"][\"login\"]}')
    print(f'Created: {latest[\"created_at\"]}')
    print(f'')
    print(latest['body'][:500])
    if len(latest['body']) > 500:
        print('... (truncated)')
    print('---')
" 2>/dev/null
else
    echo "✅ No new comments in issue #11"
fi
echo ""

echo "=========================================="
echo "Monitor check complete"
echo "=========================================="
echo ""