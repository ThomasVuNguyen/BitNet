# 🍴 Setting Up Your Own llama.cpp Fork

## Step 1: Create Your Fork ✅ DONE!

You've already created your fork at:
**https://github.com/ThomasVuNguyen/bitnet-rpi-llama.cpp**

Great job! 🎉

## Step 2: Run the Update Script

After creating your fork, run this command in your BitNet directory:

```bash
chmod +x update_submodule_to_fork.sh
./update_submodule_to_fork.sh
```

## Step 3: Build BitNet

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## ✅ What This Does

- ✅ Updates `.gitmodules` to use your fork: `https://github.com/ThomasVuNguyen/bitnet-rpi-llama.cpp.git`
- ✅ Removes the broken submodule
- ✅ Downloads fresh copy from your fork
- ✅ Uses latest commit from `merge-dev` branch
- ✅ Preserves all your ARM optimizations

## 🔄 Benefits

- **No more submodule errors**
- **Full control over the llama.cpp version**
- **Can apply custom patches if needed**
- **Stable reference that won't break**

## 🆘 If Something Goes Wrong

Restore the backup:
```bash
cp .gitmodules.backup .gitmodules
```

Then try the manual approach in the GitHub search results.
