# v1: arm dot prod instruction + increase prefill batch size:

it gets much faster -> good

# v2: (attempted) memory access optimization + TL1

TL1 is written wrong 
    - The ARM TL1 kernels have fundamental NEON type bugs
    - Trying to add int8x16_t to int16x8_t (mathematically impossible)
    - That's why ARM TL1 was disabled by default
model conversion on huggingface transformers don't support that (urgh)
    - The conversion scripts throw NotImplementedError: Architecture 'BitNetForCausalLM' not supported!
    - Can't convert HuggingFace BitNet models to TL1 format

# v3: since TL1 is fucked up, this aint applicable

# v5: multi-threading -> same as v1

Conclusion so far: The performance gain mostly comes from # of parallelization of token processing