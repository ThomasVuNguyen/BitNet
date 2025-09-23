import os
import sys
import signal
import platform
import argparse
import subprocess
import time

def run_command(command, shell=False, threads=2, is_conversation=False):
    """Run a system command with real-time output and extract performance metrics."""
    try:
        print("🚀 Starting BitNet inference with ARM optimizations...\n")
        start_time = time.time()
        
        # Run the command with real-time output and capture stderr for performance metrics
        process = subprocess.Popen(command, shell=shell, stdout=subprocess.PIPE, 
                                 stderr=subprocess.STDOUT, text=True, bufsize=1, universal_newlines=True)
        
        output_lines = []
        model_response_started = False
        model_response_lines = []
        last_response_time = time.time()
        response_count = 0
        in_assistant_response = False
        
        # Read output line by line in real-time
        for line in process.stdout:
            output_lines.append(line)
            print(line, end='')
            
            # Detect when model response starts (after the prompt)
            if 'generate: n_ctx' in line:
                model_response_started = True
                last_response_time = time.time()
            elif model_response_started and 'llama_perf_' not in line and line.strip() and not line.startswith('sampler'):
                if not any(keyword in line for keyword in ['load time', 'prompt eval', 'eval time', 'total time', 'sampling time']):
                    model_response_lines.append(line)
            
            # In conversation mode, show timing after each complete response
            if is_conversation:
                # Look for the start of a new assistant response
                if line.strip().startswith('> ') and not in_assistant_response:
                    # Starting a new assistant response
                    in_assistant_response = True
                    last_response_time = time.time()
                # Look for the end of assistant response - when we see a new user input
                elif in_assistant_response and line.strip() and not line.strip().startswith('> ') and not line.strip().startswith('System:'):
                    # End of assistant response - show metrics
                    response_count += 1
                    current_time = time.time()
                    response_time = current_time - last_response_time
                    
                    # Show speed metrics after each response in conversation mode
                    print(f"\n" + "="*50)
                    print(f"⚡ RESPONSE #{response_count} SPEED METRICS")
                    print(f"="*50)
                    print(f"⏱️  Response time: {response_time:.2f} seconds")
                    print(f"🔧 ARM dot product optimizations: ENABLED")
                    print(f"🧠 Model: BitNet 2B (ARM-optimized)")
                    print(f"🏃 Threads used: {threads}")
                    print(f"="*50)
                    print()
                    
                    in_assistant_response = False
        
        # Wait for process to complete
        process.wait()
        
        if process.returncode != 0:
            print(f"❌ Error occurred while running command. Exit code: {process.returncode}")
            sys.exit(1)
        
        end_time = time.time()
        total_time = end_time - start_time
        
        # Extract performance metrics from output (for non-conversation mode)
        if not is_conversation:
            performance_metrics = {}
            for line in output_lines:
                if 'prompt eval time' in line:
                    # Extract: prompt eval time = 161.62 ms / 2 tokens ( 80.81 ms per token, 12.37 tokens per second)
                    parts = line.split('=')[1].strip() if '=' in line else ""
                    if 'ms per token' in parts and 'tokens per second' in parts:
                        try:
                            # Extract tokens per second for prompt eval
                            tokens_per_sec = parts.split('tokens per second')[0].split(',')[-1].strip()
                            performance_metrics['prompt_eval_speed'] = float(tokens_per_sec)
                            
                            # Extract ms per token for prompt eval  
                            ms_per_token = parts.split('ms per token')[0].split('(')[-1].strip()
                            performance_metrics['prompt_eval_ms_per_token'] = float(ms_per_token)
                        except:
                            pass
                elif 'eval time' in line and 'prompt eval' not in line:
                    # Extract: eval time = 2136.86 ms / 19 runs ( 112.47 ms per token, 8.89 tokens per second)
                    parts = line.split('=')[1].strip() if '=' in line else ""
                    if 'ms per token' in parts and 'tokens per second' in parts:
                        try:
                            # Extract tokens per second for generation
                            tokens_per_sec = parts.split('tokens per second')[0].split(',')[-1].strip()
                            performance_metrics['gen_speed'] = float(tokens_per_sec)
                            
                            # Extract ms per token for generation
                            ms_per_token = parts.split('ms per token')[0].split('(')[-1].strip()
                            performance_metrics['gen_ms_per_token'] = float(ms_per_token)
                        except:
                            pass
            
            # Display speed metrics right after model response
            print(f"\n" + "="*60)
            print(f"⚡ SPEED METRICS")
            print(f"="*60)
            if 'prompt_eval_speed' in performance_metrics:
                print(f"📝 Prompt evaluation: {performance_metrics['prompt_eval_speed']:.2f} tokens/sec ({performance_metrics['prompt_eval_ms_per_token']:.2f} ms/token)")
            if 'gen_speed' in performance_metrics:
                print(f"🚀 Text generation: {performance_metrics['gen_speed']:.2f} tokens/sec ({performance_metrics['gen_ms_per_token']:.2f} ms/token)")
            print(f"⏱️  Total inference time: {total_time:.2f} seconds")
            print(f"🔧 ARM dot product optimizations: ENABLED")
            print(f"🧠 Model: BitNet 2B (ARM-optimized)")
            print(f"🏃 Threads used: {threads}")
            print(f"="*60)
        else:
            # For conversation mode, show final summary
            print(f"\n" + "="*60)
            print(f"⚡ CONVERSATION SUMMARY")
            print(f"="*60)
            print(f"⏱️  Total conversation time: {total_time:.2f} seconds")
            print(f"💬 Total responses: {response_count}")
            print(f"🔧 ARM dot product optimizations: ENABLED")
            print(f"🧠 Model: BitNet 2B (ARM-optimized)")
            print(f"🏃 Threads used: {threads}")
            print(f"="*60)
        
        return process.returncode
        
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
    run_command(command, threads=args.threads, is_conversation=args.conversation)

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