#!/bin/bash

echo "=========================================="
echo "Gomoku Server Network Diagnostic Tool"
echo "=========================================="
echo ""

# Test 1: Check if server is running
echo "Test 1: Checking if server is running..."
if ps aux | grep gomoku_server | grep -v grep > /dev/null; then
    echo "✅ Server is running"
    ps aux | grep gomoku_server | grep -v grep
else
    echo "❌ Server is NOT running"
    exit 1
fi
echo ""

# Test 2: Check if port 8888 is listening
echo "Test 2: Checking if port 8888 is listening..."
if ss -tlnp 2>/dev/null | grep 8888 > /dev/null; then
    echo "✅ Port 8888 is listening"
    ss -tlnp 2>/dev/null | grep 8888
else
    echo "❌ Port 8888 is NOT listening"
    exit 1
fi
echo ""

# Test 3: Check firewall status
echo "Test 3: Checking firewall status..."
echo "iptables chains:"
sudo iptables -L -n 2>/dev/null | grep -E "Chain|policy"
echo ""

# Test 4: Check network interfaces
echo "Test 4: Checking network interfaces..."
ip addr show 2>/dev/null | grep -E "inet |192.168"
echo ""

# Test 5: Test local connection to 127.0.0.1
echo "Test 5: Testing local connection to 127.0.0.1:8888..."
if timeout 3 nc -zv 127.0.0.1 8888 2>&1 | grep -q "succeeded"; then
    echo "✅ Local connection successful"
else
    echo "❌ Local connection failed"
fi
echo ""

# Test 6: Test connection to server IP
echo "Test 6: Testing connection to 192.168.215.125:8888..."
if timeout 3 nc -zv 192.168.215.125 8888 2>&1 | grep -q "succeeded"; then
    echo "✅ Server IP connection successful"
else
    echo "❌ Server IP connection failed"
fi
echo ""

# Test 7: Check MySQL connection
echo "Test 7: Checking MySQL connection..."
if mysqladmin ping -h 127.0.0.1 -u root 2>/dev/null | grep -q "alive"; then
    echo "✅ MySQL is running"
else
    echo "❌ MySQL is NOT running"
fi
echo ""

# Test 8: Check Redis connection
echo "Test 8: Checking Redis connection..."
if redis-cli ping 2>/dev/null | grep -q "PONG"; then
    echo "✅ Redis is running"
else
    echo "❌ Redis is NOT running"
fi
echo ""

# Test 9: Display recent server log
echo "Test 9: Recent server log entries..."
tail -10 /root/ai/server/logs/gomoku_server.log 2>/dev/null || echo "No log file found"
echo ""

echo "=========================================="
echo "Diagnostic Complete"
echo "=========================================="
echo ""
echo "Summary:"
echo "Server IP: 192.168.215.125"
echo "Server Port: 8888"
echo "Protocol: TCP"
echo "Message Format: Protobuf"
echo ""
echo "For client connection test, use:"
echo "telnet 192.168.215.125 8888"
echo ""
echo "Or send a simple TCP connection test:"
echo "nc -zv 192.168.215.125 8888"
echo ""