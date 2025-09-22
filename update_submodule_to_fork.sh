#!/bin/bash

# Script to update BitNet to use your own llama.cpp fork
# Run this AFTER you've created your fork on GitHub

set -e  # Exit on any error

echo "🍴 Updating BitNet to use your llama.cpp fork..."

# Check if we're in the right directory
if [ ! -f ".gitmodules" ]; then
    echo "❌ Error: .gitmodules not found. Make sure you're in the BitNet directory."
    exit 1
fi

echo "📝 Backing up current .gitmodules..."
cp .gitmodules .gitmodules.backup

echo "🗑️  Removing existing submodule..."
# Remove the submodule directory
rm -rf 3rdparty/llama.cpp

# Remove submodule from git config
git config --remove-section submodule.3rdparty/llama.cpp 2>/dev/null || true

echo "📝 Updating .gitmodules to use your fork..."
# Update .gitmodules to point to your fork
sed -i 's|url = https://github.com/Eddie-Wang1120/llama.cpp.git|url = https://github.com/ThomasVuNguyen/bitnet-rpi-llama.cpp.git|g' .gitmodules

echo "🔄 Syncing submodule configuration..."
# Sync the new URL
git submodule sync

echo "⬇️  Initializing submodule with your fork..."
# Initialize and update the submodule
git submodule update --init --recursive --remote

echo "✅ Done! BitNet now uses your llama.cpp fork."
echo ""
echo "📋 Summary:"
echo "   - Backup created: .gitmodules.backup"
echo "   - Submodule now points to: https://github.com/ThomasVuNguyen/bitnet-rpi-llama.cpp.git"
echo "   - Using latest commit from merge-dev branch"
echo ""
echo "🚀 You can now build BitNet with: mkdir build && cd build && cmake .. && make -j$(nproc)"
