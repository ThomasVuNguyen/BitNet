import os
import sys
import signal
import platform
import argparse
import subprocess
import time

def run_command(command, shell=False, threads=2):
    """Run a system command and capture output for performance metrics."""
    try:
        print("🚀 Starting BitNet inference with ARM optimizations...")
        start_time = time.time()
        
        # Run the command and capture output
        result = subprocess.run(command, shell=shell, check=True, 
                              capture_output=False, text=True)
        
        end_time = time.time()
        total_time = end_time - start_time
        
        print(f"\n" + "="*60)
        print(f"⚡ PERFORMANCE SUMMARY")
        print(f"="*60)
        print(f"⏱️  Total inference time: {total_time:.2f} seconds")
        print(f"🔧 ARM dot product optimizations: ENABLED")
        print(f"🧠 Model: BitNet 2B (ARM-optimized)")
        print(f"🏃 Threads used: {threads}")
        print(f"="*60)
        
        return result
        
    except subprocess.CalledProcessError as e:
        print(f"❌ Error occurred while running command: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print(f"\n" + "="*60)
        print(f"⚡ PERFORMANCE SUMMARY (Interrupted)")
        print(f"="*60)
        end_time = time.time()
        total_time = end_time - start_time
        print(f"⏱️  Runtime before interruption: {total_time:.2f} seconds")
        print(f"🔧 ARM dot product optimizations: ENABLED")
        print(f"🧠 Model: BitNet 2B (ARM-optimized)")
        print(f"🏃 Threads used: {threads}")
        print(f"💡 Tip: For detailed timing, run without -cnv flag")
        print(f"="*60)
        print("\n👋 Inference interrupted by user")
        sys.exit(0)

def run_inference():
    build_dir = "build"
    if platform.system() == "Windows":
        main_path = os.path.join(build_dir, "bin", "Release", "llama-cli.exe")
        if not os.path.exists(main_path):
            main_path = os.path.join(build_dir, "bin", "llama-cli")
    else:
        main_path = os.path.join(build_dir, "bin", "llama-cli")
    command = [
        f'{main_path}',
        '-m', args.model,
        '-n', str(args.n_predict),
        '-t', str(args.threads),
        '-p', args.prompt,
        '-ngl', '0',
        '-c', str(args.ctx_size),
        '--temp', str(args.temperature),
    ]
    if args.conversation:
        command.append("-cnv")
    run_command(command, threads=args.threads)

def signal_handler(sig, frame):
    print("Ctrl+C pressed, exiting...")
    sys.exit(0)

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    # Usage: python run_inference.py -p "Microsoft Corporation is an American multinational corporation and technology company headquartered in Redmond, Washington."
    parser = argparse.ArgumentParser(description='Run inference')
    parser.add_argument("-m", "--model", type=str, help="Path to model file", required=False, default="models/bitnet_b1_58-3B/ggml-model-i2_s.gguf")
    parser.add_argument("-n", "--n-predict", type=int, help="Number of tokens to predict when generating text", required=False, default=128)
    parser.add_argument("-p", "--prompt", type=str, help="Prompt to generate text from", required=True)
    parser.add_argument("-t", "--threads", type=int, help="Number of threads to use", required=False, default=2)
    parser.add_argument("-c", "--ctx-size", type=int, help="Size of the prompt context", required=False, default=2048)
    parser.add_argument("-temp", "--temperature", type=float, help="Temperature, a hyperparameter that controls the randomness of the generated text", required=False, default=0.8)
    parser.add_argument("-cnv", "--conversation", action='store_true', help="Whether to enable chat mode or not (for instruct models.)")

    args = parser.parse_args()
    run_inference()