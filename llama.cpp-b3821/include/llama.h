#ifndef LLAMA_H
#define LLAMA_H

#include "ggml.h"
#include "ggml-backend.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef LLAMA_SHARED
#    if defined(_WIN32) && !defined(__MINGW32__)
#        ifdef LLAMA_BUILD
#            define LLAMA_API __declspec(dllexport)
#        else
#            define LLAMA_API __declspec(dllimport)
#        endif
#    else
#        define LLAMA_API __attribute__ ((visibility ("default")))
#    endif
#else
#    define LLAMA_API
#endif

#ifdef __GNUC__
#    define DEPRECATED(func, hint) func __attribute__((deprecated(hint)))
#elif defined(_MSC_VER)
#    define DEPRECATED(func, hint) __declspec(deprecated(hint)) func
#else
#    define DEPRECATED(func, hint) func
#endif

#define LLAMA_DEFAULT_SEED 0xFFFFFFFF

// TODO: use everywhere in the implementation
#define LLAMA_TOKEN_NULL -1

#define LLAMA_FILE_MAGIC_GGLA 0x67676c61u // 'ggla'
#define LLAMA_FILE_MAGIC_GGSN 0x6767736eu // 'ggsn'
#define LLAMA_FILE_MAGIC_GGSQ 0x67677371u // 'ggsq'

#define LLAMA_SESSION_MAGIC   LLAMA_FILE_MAGIC_GGSN
#define LLAMA_SESSION_VERSION 9

#define LLAMA_STATE_SEQ_MAGIC   LLAMA_FILE_MAGIC_GGSQ
#define LLAMA_STATE_SEQ_VERSION 2

#ifdef __cplusplus
extern "C" {
#endif

    //
    // C interface
    //
    // TODO: show sample usage
    //

    // struct llama_vocab; // TODO: add in the future
    struct llama_model;
    struct llama_context;
    struct llama_sampler;

    typedef int32_t llama_pos;
    typedef int32_t llama_token;
    typedef int32_t llama_seq_id;

    enum llama_vocab_type {
        LLAMA_VOCAB_TYPE_NONE = 0, // For models without vocab
        LLAMA_VOCAB_TYPE_SPM  = 1, // LLaMA tokenizer based on byte-level BPE with byte fallback
        LLAMA_VOCAB_TYPE_BPE  = 2, // GPT-2 tokenizer based on byte-level BPE
        LLAMA_VOCAB_TYPE_WPM  = 3, // BERT tokenizer based on WordPiece
        LLAMA_VOCAB_TYPE_UGM  = 4, // T5 tokenizer based on Unigram
        LLAMA_VOCAB_TYPE_RWKV = 5, // RWKV tokenizer based on greedy tokenization
    };

    // pre-tokenization types
    enum llama_vocab_pre_type {
        LLAMA_VOCAB_PRE_TYPE_DEFAULT        = 0,
        LLAMA_VOCAB_PRE_TYPE_LLAMA3         = 1,
        LLAMA_VOCAB_PRE_TYPE_DEEPSEEK_LLM   = 2,
        LLAMA_VOCAB_PRE_TYPE_DEEPSEEK_CODER = 3,
        LLAMA_VOCAB_PRE_TYPE_FALCON         = 4,
        LLAMA_VOCAB_PRE_TYPE_MPT            = 5,
        LLAMA_VOCAB_PRE_TYPE_STARCODER      = 6,
        LLAMA_VOCAB_PRE_TYPE_GPT2           = 7,
        LLAMA_VOCAB_PRE_TYPE_REFACT         = 8,
        LLAMA_VOCAB_PRE_TYPE_COMMAND_R      = 9,
        LLAMA_VOCAB_PRE_TYPE_STABLELM2      = 10,
        LLAMA_VOCAB_PRE_TYPE_QWEN2          = 11,
        LLAMA_VOCAB_PRE_TYPE_OLMO           = 12,
        LLAMA_VOCAB_PRE_TYPE_DBRX           = 13,
        LLAMA_VOCAB_PRE_TYPE_SMAUG          = 14,
        LLAMA_VOCAB_PRE_TYPE_PORO           = 15,
        LLAMA_VOCAB_PRE_TYPE_CHATGLM3       = 16,
        LLAMA_VOCAB_PRE_TYPE_CHATGLM4       = 17,
        LLAMA_VOCAB_PRE_TYPE_VIKING         = 18,
        LLAMA_VOCAB_PRE_TYPE_JAIS           = 19,
        LLAMA_VOCAB_PRE_TYPE_TEKKEN         = 20,
        LLAMA_VOCAB_PRE_TYPE_SMOLLM         = 21,
        LLAMA_VOCAB_PRE_TYPE_CODESHELL      = 22,
        LLAMA_VOCAB_PRE_TYPE_BLOOM          = 23,
        LLAMA_VOCAB_PRE_TYPE_GPT3_FINNISH   = 24,
        LLAMA_VOCAB_PRE_TYPE_EXAONE         = 25,
    };

    enum llama_rope_type {
        LLAMA_ROPE_TYPE_NONE = -1,
        LLAMA_ROPE_TYPE_NORM = 0,
        LLAMA_ROPE_TYPE_NEOX = GGML_ROPE_TYPE_NEOX,
    };

    enum llama_token_type { //TODO: remove, required until per token attributes are available from GGUF file
        LLAMA_TOKEN_TYPE_UNDEFINED    = 0,
        LLAMA_TOKEN_TYPE_NORMAL       = 1,
        LLAMA_TOKEN_TYPE_UNKNOWN      = 2,
        LLAMA_TOKEN_TYPE_CONTROL      = 3,
        LLAMA_TOKEN_TYPE_USER_DEFINED = 4,
        LLAMA_TOKEN_TYPE_UNUSED       = 5,
        LLAMA_TOKEN_TYPE_BYTE         = 6,
    };

    enum llama_token_attr {
        LLAMA_TOKEN_ATTR_UNDEFINED    = 0,
        LLAMA_TOKEN_ATTR_UNKNOWN      = 1 << 0,
        LLAMA_TOKEN_ATTR_UNUSED       = 1 << 1,
        LLAMA_TOKEN_ATTR_NORMAL       = 1 << 2,
        LLAMA_TOKEN_ATTR_CONTROL      = 1 << 3,  // SPECIAL?
        LLAMA_TOKEN_ATTR_USER_DEFINED = 1 << 4,
        LLAMA_TOKEN_ATTR_BYTE         = 1 << 5,
        LLAMA_TOKEN_ATTR_NORMALIZED   = 1 << 6,
        LLAMA_TOKEN_ATTR_LSTRIP       = 1 << 7,
        LLAMA_TOKEN_ATTR_RSTRIP       = 1 << 8,
        LLAMA_TOKEN_ATTR_SINGLE_WORD  = 1 << 9,
    };

    // model file types
    enum llama_ftype {
        LLAMA_FTYPE_ALL_F32              = 0,
        LLAMA_FTYPE_MOSTLY_F16           = 1,  // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q4_0          = 2,  // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q4_1          = 3,  // except 1d tensors
        // LLAMA_FTYPE_MOSTLY_Q4_1_SOME_F16 = 4,  // tok_embeddings.weight and output.weight are F16
        // LLAMA_FTYPE_MOSTLY_Q4_2       = 5,  // support has been removed
        // LLAMA_FTYPE_MOSTLY_Q4_3       = 6,  // support has been removed
        LLAMA_FTYPE_MOSTLY_Q8_0          = 7,  // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q5_0          = 8,  // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q5_1          = 9,  // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q2_K          = 10, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q3_K_S        = 11, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q3_K_M        = 12, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q3_K_L        = 13, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q4_K_S        = 14, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q4_K_M        = 15, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q5_K_S        = 16, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q5_K_M        = 17, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q6_K          = 18, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ2_XXS       = 19, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ2_XS        = 20, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q2_K_S        = 21, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ3_XS        = 22, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ3_XXS       = 23, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ1_S         = 24, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ4_NL        = 25, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ3_S         = 26, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ3_M         = 27, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ2_S         = 28, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ2_M         = 29, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ4_XS        = 30, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_IQ1_M         = 31, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_BF16          = 32, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q4_0_4_4      = 33, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q4_0_4_8      = 34, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_Q4_0_8_8      = 35, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_TQ1_0         = 36, // except 1d tensors
        LLAMA_FTYPE_MOSTLY_TQ2_0         = 37, // except 1d tensors

        LLAMA_FTYPE_GUESSED = 1024, // not specified in the model file
    };

    enum llama_rope_scaling_type {
        LLAMA_ROPE_SCALING_TYPE_UNSPECIFIED = -1,
        LLAMA_ROPE_SCALING_TYPE_NONE        = 0,
        LLAMA_ROPE_SCALING_TYPE_LINEAR      = 1,
        LLAMA_ROPE_SCALING_TYPE_YARN        = 2,
        LLAMA_ROPE_SCALING_TYPE_MAX_VALUE   = LLAMA_ROPE_SCALING_TYPE_YARN,
    };

    enum llama_pooling_type {
        LLAMA_POOLING_TYPE_UNSPECIFIED = -1,
        LLAMA_POOLING_TYPE_NONE = 0,
        LLAMA_POOLING_TYPE_MEAN = 1,
        LLAMA_POOLING_TYPE_CLS  = 2,
        LLAMA_POOLING_TYPE_LAST = 3,
    };

    enum llama_attention_type {
        LLAMA_ATTENTION_TYPE_UNSPECIFIED = -1,
        LLAMA_ATTENTION_TYPE_CAUSAL      = 0,
        LLAMA_ATTENTION_TYPE_NON_CAUSAL  = 1,
    };

    enum llama_split_mode {
        LLAMA_SPLIT_MODE_NONE    = 0, // single GPU
        LLAMA_SPLIT_MODE_LAYER   = 1, // split layers and KV across GPUs
        LLAMA_SPLIT_MODE_ROW     = 2, // split rows across GPUs
    };

    // TODO: simplify (https://github.com/ggerganov/llama.cpp/pull/9294#pullrequestreview-2286561979)
    typedef struct llama_token_data {
        llama_token id; // token id
        float logit;    // log-odds of the token
        float p;        // probability of the token
    } llama_token_data;

    typedef struct llama_token_data_array {
        // TODO: consider SoA
        llama_token_data * data;
        size_t size;
        int64_t selected; // this is the index in the data array (i.e. not the token id)
        bool sorted;
    } llama_token_data_array;

    typedef bool (*llama_progress_callback)(float progress, void * user_data);

    // Input data for llama_decode
    // A llama_batch object can contain input about one or many sequences
    // The provided arrays (i.e. token, embd, pos, etc.) must have size of n_tokens
    //
    // - token  : the token ids of the input (used when embd is NULL)
    // - embd   : token embeddings (i.e. float vector of size n_embd) (used when token is NULL)
    // - pos    : the positions of the respective token in the sequence
    // - seq_id : the sequence to which the respective token belongs
    // - logits : if zero, the logits (and/or the embeddings) for the respective token will not be output
    //
    typedef struct llama_batch {
        int32_t n_tokens;

        llama_token  *  token;
        float        *  embd;
        llama_pos    *  pos;
        int32_t      *  n_seq_id;
        llama_seq_id ** seq_id;
        int8_t       *  logits; // TODO: rename this to "output"

        // NOTE: helpers for smooth API transition - can be deprecated in the future
        //       for future-proof code, use the above fields instead and ignore everything below
        //
        // pos[i] = all_pos_0 + i*all_pos_1
        //
        llama_pos    all_pos_0;  // used if pos == NULL
        llama_pos    all_pos_1;  // used if pos == NULL
        llama_seq_id all_seq_id; // used if seq_id == NULL
    } llama_batch;

    enum llama_model_kv_override_type {
        LLAMA_KV_OVERRIDE_TYPE_INT,
        LLAMA_KV_OVERRIDE_TYPE_FLOAT,
        LLAMA_KV_OVERRIDE_TYPE_BOOL,
        LLAMA_KV_OVERRIDE_TYPE_STR,
    };

    struct llama_model_kv_override {
        enum llama_model_kv_override_type tag;

        char key[128];

        union {
            int64_t val_i64;
            double  val_f64;
            bool    val_bool;
            char    val_str[128];
        };
    };

    struct llama_model_params {
        int32_t n_gpu_layers; // number of layers to store in VRAM
        enum llama_split_mode split_mode; // how to split the model across multiple GPUs

        // main_gpu interpretation depends on split_mode:
        // LLAMA_SPLIT_MODE_NONE: the GPU that is used for the entire model
        // LLAMA_SPLIT_MODE_ROW: the GPU that is used for small tensors and intermediate results
        // LLAMA_SPLIT_MODE_LAYER: ignored
        int32_t main_gpu;

        // proportion of the model (layers or rows) to offload to each GPU, size: llama_max_devices()
        const float * tensor_split;

        // comma separated list of RPC servers to use for offloading
        const char * rpc_servers;

        // Called with a progress value between 0.0 and 1.0. Pass NULL to disable.
        // If the provided progress_callback returns true, model loading continues.
        // If it returns false, model loading is immediately aborted.
        llama_progress_callback progress_callback;

        // context pointer passed to the progress callback
        void * progress_callback_user_data;

        // override key-value pairs of the model meta data
        const struct llama_model_kv_override * kv_overrides;

        // Keep the booleans together to avoid misalignment during copy-by-value.
        bool vocab_only;    // only load the vocabulary, no weights
        bool use_mmap;      // use mmap if possible
        bool use_mlock;     // force system to keep model in RAM
        bool check_tensors; // validate model tensor data
    };

    // NOTE: changing the default values of parameters marked as [EXPERIMENTAL] may cause crashes or incorrect results in certain configurations
    //       https://github.com/ggerganov/llama.cpp/pull/7544
    struct llama_context_params {
        uint32_t n_ctx;             // text context, 0 = from model
        uint32_t n_batch;           // logical maximum batch size that can be submitted to llama_decode
        uint32_t n_ubatch;          // physical maximum batch size
        uint32_t n_seq_max;         // max number of sequences (i.e. distinct states for recurrent models)
        int32_t  n_threads;         // number of threads to use for generation
        int32_t  n_threads_batch;   // number of threads to use for batch processing

        enum llama_rope_scaling_type rope_scaling_type; // RoPE scaling type, from `enum llama_rope_scaling_type`
        enum llama_pooling_type      pooling_type;      // whether to pool (sum) embedding results by sequence id
        enum llama_attention_type    attention_type;    // attention type to use for embeddings

        // ref: https://github.com/ggerganov/llama.cpp/pull/2054
        float    rope_freq_base;   // RoPE base frequency, 0 = from model
        float    rope_freq_scale;  // RoPE frequency scaling factor, 0 = from model
        float    yarn_ext_factor;  // YaRN extrapolation mix factor, negative = from model
        float    yarn_attn_factor; // YaRN magnitude scaling factor
        float    yarn_beta_fast;   // YaRN low correction dim
        float    yarn_beta_slow;   // YaRN high correction dim
        uint32_t yarn_orig_ctx;    // YaRN original context size
        float    defrag_thold;     // defragment the KV cache if holes/size > thold, < 0 disabled (default)

        ggml_backend_sched_eval_callback cb_eval;
        void * cb_eval_user_data;

        enum ggml_type type_k; // data type for K cache [EXPERIMENTAL]
        enum ggml_type type_v; // data type for V cache [EXPERIMENTAL]

        // Keep the booleans together and at the end of the struct to avoid misalignment during copy-by-value.
        // TODO: move at the end of the struct
        bool logits_all;  // the llama_decode() call computes all logits, not just the last one (DEPRECATED - set llama_batch.logits instead)
        bool embeddings;  // if true, extract embeddings (together with logits)
        bool offload_kqv; // whether to offload the KQV ops (including the KV cache) to GPU
        bool flash_attn;  // whether to use flash attention [EXPERIMENTAL]
        bool no_perf;     // whether to measure performance timings

        // Abort callback
        // if it returns true, execution of llama_decode() will be aborted
        // currently works only with CPU execution
        ggml_abort_callback abort_callback;
        void *              abort_callback_data;
    };

    // model quantization parameters
    typedef struct llama_model_quantize_params {
        int32_t nthread;                     // number of threads to use for quantizing, if <=0 will use std::thread::hardware_concurrency()
        enum llama_ftype ftype;              // quantize to this llama_ftype
        enum ggml_type output_tensor_type;   // output tensor type
        enum ggml_type token_embedding_type; // token embeddings tensor type
        bool allow_requantize;               // allow quantizing non-f32/f16 tensors
        bool quantize_output_tensor;         // quantize output.weight
        bool only_copy;                      // only copy tensors - ftype, allow_requantize and quantize_output_tensor are ignored
        bool pure;                           // quantize all tensors to the default type
        bool keep_split;                     // quantize to the same number of shards
        void * imatrix;                      // pointer to importance matrix data
        void * kv_overrides;                 // pointer to vector containing overrides
    } llama_model_quantize_params;

    typedef struct llama_logit_bias {
        llama_token token;
        float bias;
    } llama_logit_bias;

    typedef struct llama_sampler_chain_params {
        bool no_perf; // whether to measure performance timings
    } llama_sampler_chain_params;

    // used in chat template
    typedef struct llama_chat_message {
        const char * role;
        const char * content;
    } llama_chat_message;

    // lora adapter
    struct llama_lora_adapter;

    // Helpers for getting default parameters
    // TODO: update API to start accepting pointers to params structs (https://github.com/ggerganov/llama.cpp/discussions/9172)
    LLAMA_API struct llama_model_params          llama_model_default_params(void);
    LLAMA_API struct llama_context_params        llama_context_default_params(void);
    LLAMA_API struct llama_sampler_chain_params  llama_sampler_chain_default_params(void);
    LLAMA_API struct llama_model_quantize_params llama_model_quantize_default_params(void);

    // Initialize the llama + ggml backend
    // If numa is true, use NUMA optimizations
    // Call once at the start of the program
    LLAMA_API void llama_backend_init(bool numa); // Fix this line.

    //optional:
    LLAMA_API void llama_numa_init(enum ggml_numa_strategy numa);

    // Optional: an auto threadpool gets created in ggml if not passed explicitly
    LLAMA_API void llama_attach_threadpool(
               struct   llama_context * ctx,
            ggml_threadpool_t   threadpool,
            ggml_threadpool_t   threadpool_batch);
    LLAMA_API void llama_detach_threadpool(struct llama_context * ctx);

    // Call once at the end of the program - currently only used for MPI
    LLAMA_API void llama_backend_free(void);

    LLAMA_API struct llama_model * llama_load_model_from_file(
                             const char * path_model,
              struct llama_model_params   params);

    LLAMA_API void llama_free_model(struct llama_model * model);

    // TODO: rename to llama_init_from_model
    LLAMA_API struct llama_context * llama_new_context_with_model(
                     struct llama_model * model,
            struct llama_context_params   params);

    // Frees all allocated memory
    LLAMA_API void llama_free(struct llama_context * ctx);

    LLAMA_API int64_t llama_time_us(void);

    LLAMA_API size_t llama_max_devices(void);

    LLAMA_API bool llama_supports_mmap       (void);
    LLAMA_API bool llama_supports_mlock      (void);
    LLAMA_API bool llama_supports_gpu_offload(void);

    LLAMA_API uint32_t llama_n_ctx      (const struct llama_context * ctx);
    LLAMA_API uint32_t llama_n_batch    (const struct llama_context * ctx);
    LLAMA_API uint32_t llama_n_ubatch   (const struct llama_context * ctx);
    LLAMA_API uint32_t llama_n_seq_max  (const struct llama_context * ctx);

    LLAMA_API int32_t llama_n_vocab    (const struct llama_model * model);
    LLAMA_API int32_t llama_n_ctx_train(const struct llama_model * model);
    LLAMA_API int32_t llama_n_embd     (const struct llama_model * model);
    LLAMA_API int32_t llama_n_layer    (const struct llama_model * model);
    LLAMA_API int32_t llama_n_head     (const struct llama_model * model);

    LLAMA_API const struct llama_model * llama_get_model(const struct llama_context * ctx);

    LLAMA_API enum llama_pooling_type llama_pooling_type(const struct llama_context * ctx);
    LLAMA_API enum llama_vocab_type   llama_vocab_type  (const struct llama_model * model);
    LLAMA_API enum llama_rope_type    llama_rope_type   (const struct llama_model * model);

    // Get the model's RoPE frequency scaling factor
    LLAMA_API float llama_rope_freq_scale_train(const struct llama_model * model);

    // Functions to access the model's GGUF metadata scalar values
    // - The functions return the length of the string on success, or -1 on failure
    // - The output string is always null-terminated and cleared on failure
    // - GGUF array values are not supported by these functions

    // Get metadata value as a string by key name
    LLAMA_API int32_t llama_model_meta_val_str(const struct llama_model * model, const char * key, char * buf, size_t buf_size);

    // Get the number of metadata key/value pairs
    LLAMA_API int32_t llama_model_meta_count(const struct llama_model * model);

    // Get metadata key name by index
    LLAMA_API int32_t llama_model_meta_key_by_index(const struct llama_model * model, int32_t i, char * buf, size_t buf_size);

    // Get metadata value as a string by index
    LLAMA_API int32_t llama_model_meta_val_str_by_index(const struct llama_model * model, int32_t i, char * buf, size_t buf_size);

    // Get a string describing the model type
    LLAMA_API int32_t llama_model_desc(const struct llama_model * model, char * buf, size_t buf_size);

    // Returns the total size of all the tensors in the model in bytes
    LLAMA_API uint64_t llama_model_size(const struct llama_model * model);

    // Returns the total number of parameters in the model
    LLAMA_API uint64_t llama_model_n_params(const struct llama_model * model);

    // Get a llama model tensor
    LLAMA_API struct ggml_tensor * llama_get_model_tensor(struct llama_model * model, const char * name);

    // Returns true if the model contains an encoder that requires llama_encode() call
    LLAMA_API bool llama_model_has_encoder(const struct llama_model * model);

    // Returns true if the model contains a decoder that requires llama_decode() call
    LLAMA_API bool llama_model_has_decoder(const struct llama_model * model);

    // For encoder-decoder models, this function returns id of the token that must be provided
    // to the decoder to start generating output sequence. For other models, it returns -1.
    LLAMA_API llama_token llama_model_decoder_start_token(const struct llama_model * model);

    // Returns true if the model is recurrent (like Mamba, RWKV, etc.)
    LLAMA_API bool llama_model_is_recurrent(const struct llama_model * model);

    // Returns 0 on success
    LLAMA_API uint32_t llama_model_quantize(
            const char * fname_inp,
            const char * fname_out,
            const llama_model_quantize_params * params);

    // Load a LoRA adapter from file
    // The loaded adapter will be associated to the given model, and will be free when the model is deleted
    LLAMA_API struct llama_lora_adapter * llama_lora_adapter_init(
            struct llama_model * model,
            const char * path_lora);

    // Add a loaded LoRA adapter to given context
    // This will not modify model's weight
    LLAMA_API int32_t llama_lora_adapter_set(
            struct llama_context * ctx,
            struct llama_lora_adapter * adapter,
            float scale);

    // Remove a specific LoRA adapter from given context
    // Return -1 if the adapter is not present in the context
    LLAMA_API int32_t llama_lora_adapter_remove(
            struct llama_context * ctx,
            struct llama_lora_adapter * adapter);

    // Remove all LoRA adapters from given context
    LLAMA_API void llama_lora_adapter_clear(
            struct llama_context * ctx);

    // Manually free a LoRA adapter
    // Note: loaded adapters will be free when the associated model is deleted
    LLAMA_API void llama_lora_adapter_free(struct llama_lora_adapter * adapter);

    // Apply a loaded control vector to a llama_context, or if data is NULL, clear
    // the currently loaded vector.
    // n_embd should be the size of a single layer's control, and data should point
    // to an n_embd x n_layers buffer starting from layer 1.
    // il_start and il_end are the layer range the vector should apply to (both inclusive)
    // See llama_control_vector_load in common to load a control vector.
    LLAMA_API int32_t llama_control_vector_apply(
            struct llama_context * lctx,
                     const float * data,
                          size_t   len,
                         int32_t   n_embd,
                         int32_t   il_start,
                         int32_t   il_end);

    //
    // KV cache
    //

    // Information associated with an individual cell in the KV cache view.
    struct llama_kv_cache_view_cell {
        // The position for this cell. Takes KV cache shifts into account.
        // May be negative if the cell is not populated.
        llama_pos pos;
    };

    // An updateable view of the KV cache.
    struct llama_kv_cache_view {
        // Number of KV cache cells. This will be the same as the context size.
        int32_t n_cells;

        // Maximum number of sequences that can exist in a cell. It's not an error
        // if there are more sequences in a cell than this value, however they will
        // not be visible in the view cells_sequences.
        int32_t n_seq_max;

        // Number of tokens in the cache. For example, if there are two populated
        // cells, the first with 1 sequence id in it and the second with 2 sequence
        // ids then you'll have 3 tokens.
        int32_t token_count;

        // Number of populated cache cells.
        int32_t used_cells;

        // Maximum contiguous empty slots in the cache.
        int32_t max_contiguous;

        // Index to the start of the max_contiguous slot range. Can be negative
        // when cache is full.
        int32_t max_contiguous_idx;

        // Information for an individual cell.
        struct llama_kv_cache_view_cell * cells;

        // The sequences for each cell. There will be n_seq_max items per cell.
        llama_seq_id * cells_sequences;
    };

    // Create an empty KV cache view. (use only for debugging purposes)
    LLAMA_API struct llama_kv_cache_view llama_kv_cache_view_init(const struct llama_context * ctx, int32_t n_seq_max);

    // Free a KV cache view. (use only for debugging purposes)
    LLAMA_API void llama_kv_cache_view_free(struct llama_kv_cache_view * view);

    // Update the KV cache view structure with the current state of the KV cache. (use only for debugging purposes)
    LLAMA_API void llama_kv_cache_view_update(const struct llama_context * ctx, struct llama_kv_cache_view * view);

    // Returns the number of tokens in the KV cache (slow, use only for debug)
    // If a KV cell has multiple sequences assigned to it, it will be counted multiple times
    LLAMA_API int32_t llama_get_kv_cache_token_count(const struct llama_context * ctx);

    // Returns the number of used KV cells (i.e. have at least one sequence assigned to them)
    LLAMA_API int32_t llama_get_kv_cache_used_cells(const struct llama_context * ctx);

    // Clear the KV cache - both cell info is erased and KV data is zeroed
    LLAMA_API void llama_kv_cache_clear(
            struct llama_context * ctx);

    // Removes all tokens that belong to the specified sequence and have positions in [p0, p1)
    // Returns false if a partial sequence cannot be removed. Removing a whole sequence never fails
    // seq_id < 0 : match any sequence
    // p0 < 0     : [0,  p1]
    // p1 < 0     : [p0, inf)
    LLAMA_API bool llama_kv_cache_seq_rm(
            struct llama_context * ctx,
                    llama_seq_id   seq_id,
                       llama_pos   p0,
                       llama_pos   p1);

    // Copy all tokens that belong to the specified sequence to another sequence
    // Note that this does not allocate extra KV cache memory - it simply assigns the tokens to the new sequence
    // p0 < 0 : [0,  p1]
    // p1 < 0 : [p0, inf)
    LLAMA_API void llama_kv_cache_seq_cp(
            struct llama_context * ctx,
                    llama_seq_id   seq_id_src,
                    llama_seq_id   seq_id_dst,
                       llama_pos   p0,
                       llama_pos   p1);

    // Removes all tokens that do not belong to the specified sequence
    LLAMA_API void llama_kv_cache_seq_keep(
            struct llama_context * ctx,
                    llama_seq_id   seq_id);

    // Adds relative position "delta" to all tokens that belong to the specified sequence and have positions in [p0, p1)
    // If the KV cache is RoPEd, the KV data is updated accordingly:
    //   - lazily on next llama_decode()
    //   - explicitly with llama_kv_cache_update()
    // p0 < 0 : [0,  p1]
    // p1 < 0 : [p0, inf)
    LLAMA_API void llama_kv_cache_seq_add(
            struct llama_context * ctx,
                    llama_seq_id   seq_id,
                       llama_pos   p0,
                       llama_pos   p1,
                       llama_pos   delta);

    // Integer division of the positions by factor of `d > 1`
    // If the KV cache is RoPEd, the KV data is updated accordingly:
    //   - lazily on next llama_decode()
    //   - explicitly with llama_kv_cache_update()
    // p0 < 0 : [0,  p1]
    // p1 < 0 : [p0, inf)
    LLAMA_API void llama_kv_cache_seq_div(
            struct llama_context * ctx,
                    llama_seq_id   seq_id,
                       llama_pos   p0,
                       llama_pos   p1,
                             int   d);

    // Returns the largest position present in the KV cache for the specified sequence
    LLAMA_API llama_pos llama_kv_cache_seq_pos_max(
            struct llama_context * ctx,
                    llama_seq_id   seq_id);

    // Defragment the KV cache
    // This will be applied:
    //   - lazily on next llama_decode()
    //   - explicitly with llama_kv_cache_update()
    LLAMA_API void llama_kv_cache_defrag(struct llama_context * ctx);

    // Apply the KV cache updates (such as K-shifts, defragmentation, etc.)
    LLAMA_API void llama_kv_cache_update(struct llama_context * ctx);

    //
    // State / sessions
    //

    // Returns the *actual* size in bytes of the state
    // (logits, embedding and kv_cache)
    // Only use when saving the state, not when restoring it, otherwise the size may be too small.
    LLAMA_API size_t llama_state_get_size(struct llama_context * ctx);
    LLAMA_API DEPRECATED(size_t llama_get_state_size(struct llama_context * ctx),
        "use llama_state_get_size instead");

    // Copies the state to the specified destination address.
    // Destination needs to have allocated enough memory.
    // Returns the number of bytes copied
    LLAMA_API size_t llama_state_get_data(
            struct llama_context * ctx,
                         uint8_t * dst,
                          size_t   size);
    LLAMA_API DEPRECATED(size_t llama_copy_state_data(
            struct llama_context * ctx,
                         uint8_t * dst),
        "use llama_state_get_data instead");

    // Set the state reading from the specified address
    // Returns the number of bytes read
    LLAMA_API size_t llama_state_set_data(
            struct llama_context * ctx,
                   const uint8_t * src,
                          size_t   size);
    LLAMA_API DEPRECATED(size_t llama_set_state_data(
            struct llama_context * ctx,
                   const uint8_t * src),
        "use llama_state_set_data instead");

    // Save/load session file
    LLAMA_API bool llama_state_load_file(
            struct llama_context * ctx,
                      const char * path_session,
                     llama_token * tokens_out,
                          size_t   n_token_capacity,
                          size_t * n_token_count_out);
    LLAMA_API DEPRECATED(bool llama_load_session_file(
            struct llama_context * ctx,
                      const char * path_session,
                     llama_token * tokens_out,
                          size_t   n_token_capacity,
                          size_t * n_token_count_out),
        "use llama_state_load_file instead");

    LLAMA_API bool llama_state_save_file(
            struct llama_context * ctx,
                      const char * path_session,
               const llama_token * tokens,
                          size_t   n_token_count);
    LLAMA_API DEPRECATED(bool llama_save_session_file(
            struct llama_context * ctx,
                      const char * path_session,
               const llama_token * tokens,
                          size_t   n_token_count),
        "use llama_state_save_file instead");

    // Get the exact size needed to copy the KV cache of a single sequence
    LLAMA_API size_t llama_state_seq_get_size(
            struct llama_context * ctx,
                    llama_seq_id   seq_id);

    // Copy the KV cache of a single sequence into the specified buffer
    LLAMA_API size_t llama_state_seq_get_data(
            struct llama_context * ctx,
                         uint8_t * dst,
                          size_t   size,
                    llama_seq_id   seq_id);

    // Copy the sequence data (originally copied with `llama_state_seq_get_data`) into the specified sequence
    // Returns:
    //  - Positive: Ok
    //  - Zero: Failed to load
    LLAMA_API size_t llama_state_seq_set_data(
            struct llama_context * ctx,
                   const uint8_t * src,
                          size_t   size,
                    llama_seq_id   dest_seq_id);

    LLAMA_API size_t llama_state_seq_save_file(
            struct llama_context * ctx,
                      const char * filepath,
                    llama_seq_id   seq_id,
               const llama_token * tokens,
                          size_t   n_token_count);

    LLAMA_API size_t llama_state_seq_load_file(
            struct llama_context * ctx,
                      const char * filepath,
                    llama_seq_id   dest_seq_id,
                     llama_token * tokens_out,
                          size_t   n_token_capacity,
                          size_t * n_token_count_out);

    //
    // Decoding
    //

    // Return batch for single sequence of tokens starting at pos_0
    //
    // NOTE: this is a helper function to facilitate transition to the new batch API - avoid using it
    //
    LLAMA_API struct llama_batch llama_batch_get_one(
                  llama_token * tokens,
                      int32_t   n_tokens,
                    llama_pos   pos_0,
                 llama_seq_id   seq_id);

    // Allocates a batch of tokens on the heap that can hold a maximum of n_tokens
    // Each token can be assigned up to n_seq_max sequence ids
    // The batch has to be freed with llama_batch_free()
    // If embd != 0, llama_batch.embd will be allocated with size of n_tokens * embd * sizeof(float)
    // Otherwise, llama_batch.token will be allocated to store n_tokens llama_token
    // The rest of the llama_batch members are allocated with size n_tokens
    // All members are left uninitialized
    LLAMA_API struct llama_batch llama_batch_init(
            int32_t n_tokens,
            int32_t embd,
            int32_t n_seq_max);

    // Frees a batch of tokens allocated with llama_batch_init()
    LLAMA_API void llama_batch_free(struct llama_batch batch);

    // Processes a batch of tokens with the ecoder part of the encoder-decoder model.
    // Stores the encoder output internally for later use by the decoder cross-attention layers.
    //   0 - success
    // < 0 - error
    LLAMA_API int32_t llama_encode(
            struct llama_context * ctx,
              struct llama_batch   batch);

    // Positive return values does not mean a fatal error, but rather a warning.
    //   0 - success
    //   1 - could not find a KV slot for the batch (try reducing the size of the batch or increase the context)
    // < 0 - error
    LLAMA_API int32_t llama_decode(
            struct llama_context * ctx,
              struct llama_batch   batch);

    // Set the number of threads used for decoding
    // n_threads is the number of threads used for generation (single token)
    // n_threads_batch is the number of threads used for prompt and batch processing (multiple tokens)
    LLAMA_API void llama_set_n_threads(struct llama_context * ctx, int32_t n_threads, int32_t n_threads_batch);

    // Get the number of threads used for generation of a single token.
    LLAMA_API int32_t llama_n_threads(struct llama_context * ctx);

    // Get the number of threads used for prompt and batch processing (multiple token).
    LLAMA_API int32_t llama_n_threads_batch(struct llama_context * ctx);

    // Set whether the model is in embeddings mode or not
    // If true, embeddings will be returned but logits will not
    LLAMA_API void llama_set_embeddings(struct llama_context * ctx, bool embeddings);

    // Set whether to use causal attention or not
    // If set to true, the model will only attend to the past tokens
    LLAMA_API void llama_set_causal_attn(struct llama_context * ctx, bool causal_attn);

    // Set abort callback
    LLAMA_API void llama_set_abort_callback(struct llama_context * ctx, ggml_abort_callback abort_callback, void * abort_callback_data);

    // Wait until all computations are finished
    // This is automatically done when using one of the functions below to obtain the computation results
    // and is not necessary to call it explicitly in most cases
    LLAMA_API void llama_synchronize(struct llama_context * ctx);

    // Token logits obtained from the last call to llama_decode()
    // The logits for which llama_batch.logits[i] != 0 are stored contiguously
    // in the order they have appeared in the batch.
    // Rows: number of tokens for which llama_batch.logits[i] != 0
    // Cols: n_vocab
    LLAMA_API float * llama_get_logits(struct llama_context * ctx);

    // Logits for the ith token. For positive indices, Equivalent to:
    // llama_get_logits(ctx) + ctx->output_ids[i]*n_vocab
    // Negative indicies can be used to access logits in reverse order, -1 is the last logit.
    // returns NULL for invalid ids.
    LLAMA_API float * llama_get_logits_ith(struct llama_context * ctx, int32_t i);

    // Get all output token embeddings.
    // when pooling_type == LLAMA_POOLING_TYPE_NONE or when using a generative model,
    // the embeddings for which llama_batch.logits[i] != 0 are stored contiguously
    // in the order they have appeared in the batch.
    // shape: [n_outputs*n_embd]
    // Otherwise, returns NULL.
    LLAMA_API float * llama_get_embeddings(struct llama_context * ctx);

    // Get the embeddings for the ith token. For positive indices, Equivalent to:
    // llama_get_embeddings(ctx) + ctx->output_ids[i]*n_embd
    // Negative indicies can be used to access embeddings in reverse order, -1 is the last embedding.
    // shape: [n_embd] (1-dimensional)
    // returns NULL for invalid ids.
    LLAMA_API float * llama_get_embeddings_ith(struct llama_context * ctx, int32_t i);

    // Get the embeddings for a sequence id
    // Returns NULL if pooling_type is LLAMA_POOLING_TYPE_NONE
    // shape: [n_embd] (1-dimensional)
    LLAMA_API float * llama_get_embeddings_seq(struct llama_context * ctx, llama_seq_id seq_id);

    //
    // Vocab
    //

    LLAMA_API const char * llama_token_get_text(const struct llama_model * model, llama_token token);

    LLAMA_API float llama_token_get_score(const struct llama_model * model, llama_token token);

    LLAMA_API enum llama_token_attr llama_token_get_attr(const struct llama_model * model, llama_token token);

    // Check if the token is supposed to end generation (end-of-generation, eg. EOS, EOT, etc.)
    LLAMA_API bool llama_token_is_eog(const struct llama_model * model, llama_token token);

    // Identify if Token Id is a control token or a render-able token
    LLAMA_API bool llama_token_is_control(const struct llama_model * model, llama_token token);

    // Special tokens
    LLAMA_API llama_token llama_token_bos(const struct llama_model * model); // beginning-of-sentence
    LLAMA_API llama_token llama_token_eos(const struct llama_model * model); // end-of-sentence
    LLAMA_API llama_token llama_token_cls(const struct llama_model * model); // classification
    LLAMA_API llama_token llama_token_sep(const struct llama_model * model); // sentence separator
    LLAMA_API llama_token llama_token_nl (const struct llama_model * model); // next-line
    LLAMA_API llama_token llama_token_pad(const struct llama_model * model); // padding

    LLAMA_API bool llama_add_bos_token(const struct llama_model * model);
    LLAMA_API bool llama_add_eos_token(const struct llama_model * model);

    // Codellama infill tokens
    LLAMA_API llama_token llama_token_prefix(const struct llama_model * model); // Beginning of infill prefix
    LLAMA_API llama_token llama_token_middle(const struct llama_model * model); // Beginning of infill middle
    LLAMA_API llama_token llama_token_suffix(const struct llama_model * model); // Beginning of infill suffix
    LLAMA_API llama_token llama_token_eot   (const struct llama_model * model); // End of infill middle

    //
    // Tokenization
    //

    /// @details Convert the provided text into tokens.
    /// @param tokens The tokens pointer must be large enough to hold the resulting tokens.
    /// @return Returns the number of tokens on success, no more than n_tokens_max
    /// @return Returns a negative number on failure - the number of tokens that would have been returned
    /// @param add_special Allow to add BOS and EOS tokens if model is configured to do so.
    /// @param parse_special Allow tokenizing special and/or control tokens which otherwise are not exposed and treated
    ///                      as plaintext. Does not insert a leading space.
    LLAMA_API int32_t llama_tokenize(
        const struct llama_model * model,
                      const char * text,
                         int32_t   text_len,
                     llama_token * tokens,
                         int32_t   n_tokens_max,
                            bool   add_special,
                            bool   parse_special);

    // Token Id -> Piece.
    // Uses the vocabulary in the provided context.
    // Does not write null terminator to the buffer.
    // User can skip up to 'lstrip' leading spaces before copying (useful when encoding/decoding multiple tokens with 'add_space_prefix')
    // @param special If true, special tokens are rendered in the output.
    LLAMA_API int32_t llama_token_to_piece(
              const struct llama_model * model,
                           llama_token   token,
                                  char * buf,
                               int32_t   length,
                               int32_t   lstrip,
                                  bool   special);

    /// @details Convert the provided tokens into text (inverse of llama_tokenize()).
    /// @param text The char pointer must be large enough to hold the resulting text.
    /// @return Returns the number of chars/bytes on success, no more than text_len_max.
    /// @return Returns a negative number on failure - the number of chars/bytes that would have been returned.
    /// @param remove_special Allow to remove BOS and EOS tokens if model is configured to do so.
    /// @param unparse_special If true, special tokens are rendered in the output.
    LLAMA_API int32_t llama_detokenize(
        const struct llama_model * model,
               const llama_token * tokens,
                         int32_t   n_tokens,
                            char * text,
                         int32_t   text_len_max,
                            bool   remove_special,
                            bool   unparse_special);

    //
    // Chat templates
    //

    /// Apply chat template. Inspired by hf apply_chat_template() on python.
    /// Both "model" and "custom_template" are optional, but at least one is required. "custom_template" has higher precedence than "model"
    /// NOTE: This function does not use a jinja parser. It only support a pre-defined list of template. See more: https://github.com/ggerganov/llama.cpp/wiki/Templates-supported-by-llama_chat_apply_template
    /// @param tmpl A Jinja template to use for this chat. If this is nullptr, the model’s default chat template will be used instead.
    /// @param chat Pointer to a list of multiple llama_chat_message
    /// @param n_msg Number of llama_chat_message in this chat
    /// @param add_ass Whether to end the prompt with the token(s) that indicate the start of an assistant message.
    /// @param buf A buffer to hold the output formatted prompt. The recommended alloc size is 2 * (total number of characters of all messages)
    /// @param length The size of the allocated buffer
    /// @return The total number of bytes of the formatted prompt. If is it larger than the size of buffer, you may need to re-alloc it and then re-apply the template.
    LLAMA_API int32_t llama_chat_apply_template(
              const struct llama_model * model,
                            const char * tmpl,
       const struct llama_chat_message * chat,
                                size_t   n_msg,
                                  bool   add_ass,
                                  char * buf,
                               int32_t   length);

    //
    // Sampling API
    //
    // Sample usage:
    //
    //    // prepare the sampling chain at the start
    //    auto sparams = llama_sampler_chain_default_params();
    //
    //    llama_sampler * smpl = llama_sampler_chain_init(sparams);
    //
    //    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(50));
    //    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.9, 1));
    //    llama_sampler_chain_add(smpl, llama_sampler_init_temp (0.8));
    //
    //    // typically, the chain should end with a sampler such as "greedy", "dist" or "mirostat"
    //    // this sampler will be responsible to select the actual token
    //    llama_sampler_chain_add(smpl, llama_sampler_init_dist(seed));
    //
    //    ...
    //
    //    // decoding loop:
    //    while (...) {
    //        ...
    //
    //        llama_decode(ctx, batch);
    //
    //        // sample from the logits of the last token in the batch
    //        const llama_token id = llama_sampler_sample(smpl, ctx, -1);
    //
    //        // accepting the token updates the internal state of certain samplers (e.g. grammar, repetition, etc.)
    //        llama_sampler_accept(smpl, id);
    //        ...
    //    }
    //
    //    llama_sampler_free(smpl);
    //
    // TODO: In the future, llama_sampler will be utilized to offload the sampling to the backends (e.g. GPU).
    // TODO: in the future, the entire sampling API that uses llama_model should start using llama_vocab
    //

    typedef void * llama_sampler_context_t;

    // user code can implement the interface below in order to create custom llama_sampler
    struct llama_sampler_i {
        const char *           (*name)  (const struct llama_sampler * smpl);                                 // can be NULL
        void                   (*accept)(      struct llama_sampler * smpl, llama_token token);              // can be NULL
        void                   (*apply) (      struct llama_sampler * smpl, llama_token_data_array * cur_p); // required
        void                   (*reset) (      struct llama_sampler * smpl);                                 // can be NULL
        struct llama_sampler * (*clone) (const struct llama_sampler * smpl);                                 // can be NULL if ctx is NULL
        void                   (*free)  (      struct llama_sampler * smpl);                                 // can be NULL if ctx is NULL

        // TODO: API for internal libllama usage for appending the sampling to an existing ggml_cgraph
        //void (*apply_ggml) (struct llama_sampler * smpl, ...);
    };

    struct llama_sampler {
        struct llama_sampler_i  * iface;
        llama_sampler_context_t   ctx;
    };

    // mirror of llama_sampler_i:
    LLAMA_API const char *           llama_sampler_name  (const struct llama_sampler * smpl);
    LLAMA_API void                   llama_sampler_accept(      struct llama_sampler * smpl, llama_token token);
    LLAMA_API void                   llama_sampler_apply (      struct llama_sampler * smpl, llama_token_data_array * cur_p);
    LLAMA_API void                   llama_sampler_reset (      struct llama_sampler * smpl);
    LLAMA_API struct llama_sampler * llama_sampler_clone (const struct llama_sampler * smpl);
    // important: do not free if the sampler has been added to a llama_sampler_chain (via llama_sampler_chain_add)
    LLAMA_API void                   llama_sampler_free  (      struct llama_sampler * smpl);

    // llama_sampler_chain
    // a type of llama_sampler that can chain multiple samplers one after another

    LLAMA_API struct llama_sampler * llama_sampler_chain_init(struct llama_sampler_chain_params params);

    // important: takes ownership of the sampler object and will free it when llama_sampler_free is called
    LLAMA_API void                   llama_sampler_chain_add(      struct llama_sampler * chain, struct llama_sampler * smpl);
    LLAMA_API struct llama_sampler * llama_sampler_chain_get(const struct llama_sampler * chain, int32_t i);
    LLAMA_API int                    llama_sampler_chain_n  (const struct llama_sampler * chain);

    // after removing a sampler, the chain will no longer own it, and it will not be freed when the chain is freed
    LLAMA_API struct llama_sampler * llama_sampler_chain_remove(   struct llama_sampler * chain, int32_t i);

    // available samplers:

    LLAMA_API struct llama_sampler * llama_sampler_init_greedy     (void);
    LLAMA_API struct llama_sampler * llama_sampler_init_dist       (uint32_t seed);

    /// @details Sorts candidate tokens by their logits in descending order and calculate probabilities based on logits.
    /// NOTE: Avoid using on the full vocabulary as the sorting can become slow. For example, apply top-k or top-p sampling first.
    LLAMA_API struct llama_sampler * llama_sampler_init_softmax    (void);

    /// @details Top-K sampling described in academic paper "The Curious Case of Neural Text Degeneration" https://arxiv.org/abs/1904.09751
    LLAMA_API struct llama_sampler * llama_sampler_init_top_k      (int32_t k);

    /// @details Nucleus sampling described in academic paper "The Curious Case of Neural Text Degeneration" https://arxiv.org/abs/1904.09751
    LLAMA_API struct llama_sampler * llama_sampler_init_top_p      (float   p, size_t min_keep);

    /// @details Minimum P sampling as described in https://github.com/ggerganov/llama.cpp/pull/3841
    LLAMA_API struct llama_sampler * llama_sampler_init_min_p      (float   p, size_t min_keep);

    /// @details Tail Free Sampling described in https://www.trentonbricken.com/Tail-Free-Sampling/.
    LLAMA_API struct llama_sampler * llama_sampler_init_tail_free  (float   z, size_t min_keep);

    /// @details Locally Typical Sampling implementation described in the paper https://arxiv.org/abs/2202.00666.
    LLAMA_API struct llama_sampler * llama_sampler_init_typical    (float   p, size_t min_keep);
    LLAMA_API struct llama_sampler * llama_sampler_init_temp       (float   t);

    /// @details Dynamic temperature implementation (a.k.a. entropy) described in the paper https://arxiv.org/abs/2309.02772.
    LLAMA_API struct llama_sampler * llama_sampler_init_temp_ext   (float   t, float   delta, float exponent);

    /// @details Mirostat 1.0 algorithm described in the paper https://arxiv.org/abs/2007.14966. Uses tokens instead of words.
    /// @param candidates A vector of `llama_token_data` containing the candidate tokens, their probabilities (p), and log-odds (logit) for the current position in the generated text.
    /// @param tau  The target cross-entropy (or surprise) value you want to achieve for the generated text. A higher value corresponds to more surprising or less predictable text, while a lower value corresponds to less surprising or more predictable text.
    /// @param eta The learning rate used to update `mu` based on the error between the target and observed surprisal of the sampled word. A larger learning rate will cause `mu` to be updated more quickly, while a smaller learning rate will result in slower updates.
    /// @param m The number of tokens considered in the estimation of `s_hat`. This is an arbitrary value that is used to calculate `s_hat`, which in turn helps to calculate the value of `k`. In the paper, they use `m = 100`, but you can experiment with different values to see how it affects the performance of the algorithm.
    /// @param mu Maximum cross-entropy. This value is initialized to be twice the target cross-entropy (`2 * tau`) and is updated in the algorithm based on the error between the target and observed surprisal.
    LLAMA_API struct llama_sampler * llama_sampler_init_mirostat(
                             int32_t   n_vocab,
                            uint32_t   seed,
                               float   tau,
                               float   eta,
                             int32_t   m);

    /// @details Mirostat 2.0 algorithm described in the paper https://arxiv.org/abs/2007.14966. Uses tokens instead of words.
    /// @param candidates A vector of `llama_token_data` containing the candidate tokens, their probabilities (p), and log-odds (logit) for the current position in the generated text.
    /// @param tau  The target cross-entropy (or surprise) value you want to achieve for the generated text. A higher value corresponds to more surprising or less predictable text, while a lower value corresponds to less surprising or more predictable text.
    /// @param eta The learning rate used to update `mu` based on the error between the target and observed surprisal of the sampled word. A larger learning rate will cause `mu` to be updated more quickly, while a smaller learning rate will result in slower updates.
    /// @param mu Maximum cross-entropy. This value is initialized to be twice the target cross-entropy (`2 * tau`) and is updated in the algorithm based on the error between the target and observed surprisal.
    LLAMA_API struct llama_sampler * llama_sampler_init_mirostat_v2(
                            uint32_t   seed,
                               float   tau,
                               float   eta);

    LLAMA_API struct llama_sampler * llama_sampler_init_grammar(
            const struct llama_model * model,
                          const char * grammar_str,
                          const char * grammar_root);

    LLAMA_API struct llama_sampler * llama_sampler_init_penalties(
                             int32_t   n_vocab,         // llama_n_vocab()
                         llama_token   special_eos_id,  // llama_token_eos()
                         llama_token   linefeed_id,     // llama_token_nl()
                             int32_t   penalty_last_n,  // last n tokens to penalize (0 = disable penalty, -1 = context size)
                               float   penalty_repeat,  // 1.0 = disabled
                               float   penalty_freq,    // 0.0 = disabled
                               float   penalty_present, // 0.0 = disabled
                                bool   penalize_nl,     // consider newlines as a repeatable token
                                bool   ignore_eos);     // ignore the end-of-sequence token

    LLAMA_API struct llama_sampler * llama_sampler_init_logit_bias(
                             int32_t   n_vocab,
                             int32_t   n_logit_bias,
              const llama_logit_bias * logit_bias);


    // Returns the seed used by the sampler if applicable, LLAMA_DEFAULT_SEED otherwise
    LLAMA_API uint32_t llama_sampler_get_seed(const struct llama_sampler * smpl);

    /// @details Sample and accept a token from the idx-th output of the last evaluation
    //
    // Shorthand for:
    //    const auto * logits = llama_get_logits_ith(ctx, idx);
    //    llama_token_data_array cur_p = { ... init from logits ... };
    //    llama_sampler_apply(smpl, &cur_p);
    //    auto token = cur_p.data[cur_p.selected].id;
    //    llama_sampler_accept(smpl, token);
    //    return token;
    // Returns the sampled token
    LLAMA_API llama_token llama_sampler_sample(struct llama_sampler * smpl, struct llama_context * ctx, int32_t idx);

    // TODO: extend in the future
    //LLAMA_API void llama_decode_with_sampler(struct llama_context * ctx, struct llama_sampler * smpl, struct llama_batch batch, ...);

    //
    // Model split
    //

    /// @details Build a split GGUF final path for this chunk.
    ///          llama_split_path(split_path, sizeof(split_path), "/models/ggml-model-q4_0", 2, 4) => split_path = "/models/ggml-model-q4_0-00002-of-00004.gguf"
    //  Returns the split_path length.
    LLAMA_API int llama_split_path(char * split_path, size_t maxlen, const char * path_prefix, int split_no, int split_count);

    /// @details Extract the path prefix from the split_path if and only if the split_no and split_count match.
    ///          llama_split_prefix(split_prefix, 64, "/models/ggml-model-q4_0-00002-of-00004.gguf", 2, 4) => split_prefix = "/models/ggml-model-q4_0"
    //  Returns the split_prefix length.
    LLAMA_API int llama_split_prefix(char * split_prefix, size_t maxlen, const char * split_path, int split_no, int split_count);

    // Print system information
    LLAMA_API const char * llama_print_system_info(void);

    // Set callback for all future logging events.
    // If this is not called, or NULL is supplied, everything is output on stderr.
    LLAMA_API void llama_log_set(ggml_log_callback log_callback, void * user_data);

    //
    // Performance utils
    //
    // NOTE: Used by llama.cpp examples, avoid using in third-party apps. Instead, do your own performance measurements.
    //

    struct llama_perf_context_data {
        double t_start_ms;
        double t_load_ms;
        double t_p_eval_ms;
        double t_eval_ms;

        int32_t n_p_eval;
        int32_t n_eval;
    };

    struct llama_perf_sampler_data {
        double t_sample_ms;

        int32_t n_sample;
    };

    LLAMA_API struct llama_perf_context_data llama_perf_context      (const struct llama_context * ctx);
    LLAMA_API void                           llama_perf_context_print(const struct llama_context * ctx);
    LLAMA_API void                           llama_perf_context_reset(      struct llama_context * ctx);

    // NOTE: the following work only with samplers constructed via llama_sampler_chain_init
    LLAMA_API struct llama_perf_sampler_data llama_perf_sampler      (const struct llama_sampler * chain);
    LLAMA_API void                           llama_perf_sampler_print(const struct llama_sampler * chain);
    LLAMA_API void                           llama_perf_sampler_reset(      struct llama_sampler * chain);

    LLAMA_API void llama_perf_dump_yaml(FILE * stream, const struct llama_context * ctx);

#ifdef __cplusplus
}
#endif

#endif // LLAMA_H
