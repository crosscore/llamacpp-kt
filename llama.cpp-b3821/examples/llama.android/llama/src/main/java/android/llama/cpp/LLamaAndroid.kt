// llama.cpp-b3821/examples/llama.android/llama/src/main/java/android/llama/cpp/LLamaAndroid.kt

package android.llama.cpp

import android.util.Log
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.withContext
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger
import kotlin.concurrent.thread

class LLamaAndroid {
    private val tag: String? = this::class.simpleName

    private val threadLocalState: ThreadLocal<State> = ThreadLocal.withInitial { State.Idle }

    private val runLoop: CoroutineDispatcher = Executors.newSingleThreadExecutor {
        thread(start = false, name = "Llm-RunLoop") {
            Log.d(tag, "Dedicated thread for native code: ${Thread.currentThread().name}")

            // No-op if called more than once.
            System.loadLibrary("llama-android")

            // Set llama log handler to Android
            log_to_android()
            backend_init(false)

            Log.d(tag, system_info())

            it.run()
        }.apply {
            uncaughtExceptionHandler = Thread.UncaughtExceptionHandler { _, exception: Throwable ->
                Log.e(tag, "Unhandled exception", exception)
            }
        }
    }.asCoroutineDispatcher()

    private val nlen: Int = 64

    private external fun log_to_android()
    private external fun load_model(filename: String): Long
    private external fun free_model(model: Long)
    private external fun new_context(model: Long): Long
    private external fun free_context(context: Long)
    private external fun backend_init(numa: Boolean)
    private external fun backend_free()
    private external fun new_batch(nTokens: Int, embd: Int, nSeqMax: Int): Long
    private external fun free_batch(batch: Long)
    private external fun new_sampler(): Long
    private external fun free_sampler(sampler: Long)

    private external fun system_info(): String

    private external fun completion_init(
        context: Long,
        batch: Long,
        text: String,
        nLen: Int
    ): Int

    private external fun completion_loop(
        context: Long,
        batch: Long,
        sampler: Long,
        nLen: Int,
        ncur: IntVar
    ): String?

    private external fun kv_cache_clear(context: Long)

    private val operationCount = AtomicInteger(0)
    private val isUnloading = AtomicBoolean(false)

    suspend fun load(pathToModel: String) {
        withContext(runLoop) {
            // Indicate that unloading is about to happen
            isUnloading.set(true)

            // Wait until all ongoing operations have completed
            while (operationCount.get() > 0) {
                delay(50)  // Wait for 50ms before checking again
            }

            // Now proceed to unload and load
            // Load the new model first
            val newModel = load_model(pathToModel)
            if (newModel == 0L) throw IllegalStateException("load_model() failed")

            val newContext = new_context(newModel)
            if (newContext == 0L) {
                free_model(newModel)
                throw IllegalStateException("new_context() failed")
            }

            // Unload the existing model
            when (val state = threadLocalState.get()) {
                is State.Loaded -> {
                    free_context(state.context)
                    free_model(state.model)
                }
                else -> { /* No model to unload */ }
            }

            Log.i(tag, "Loaded model $pathToModel")
            threadLocalState.set(State.Loaded(newModel, newContext))

            // Reset the unloading flag
            isUnloading.set(false)
        }
    }

    fun send(message: String): Flow<String> = flow {
        if (isUnloading.get()) {
            throw CancellationException("Operation canceled due to model unloading")
        }

        operationCount.incrementAndGet()

        try {
            when (val state = threadLocalState.get()) {
                is State.Loaded -> {
                    val batch = new_batch(512, 0, 1)
                    val sampler = new_sampler()
                    if (batch == 0L || sampler == 0L) {
                        throw IllegalStateException("Failed to create batch or sampler")
                    }
                    try {
                        val ncur = IntVar(completion_init(state.context, batch, message, nlen))
                        while (ncur.value <= nlen) {
                            // Check if the model is still loaded
                            if (threadLocalState.get() !is State.Loaded || isUnloading.get()) {
                                break
                            }
                            val str = completion_loop(state.context, batch, sampler, nlen, ncur)
                            if (str == null) {
                                break
                            }
                            emit(str)
                        }
                        kv_cache_clear(state.context)
                    } finally {
                        free_sampler(sampler)
                        free_batch(batch)
                    }
                }
                else -> {}
            }
        } finally {
            operationCount.decrementAndGet()
        }
    }.flowOn(runLoop)

    suspend fun unload() {
        withContext(runLoop) {
            // Indicate that unloading is about to happen
            isUnloading.set(true)

            // Wait until all ongoing operations have completed
            while (operationCount.get() > 0) {
                delay(50)
            }

            when (val state = threadLocalState.get()) {
                is State.Loaded -> {
                    // Free resources in the correct order
                    free_context(state.context)
                    free_model(state.model)

                    threadLocalState.set(State.Idle)
                }
                else -> {}
            }

            // Reset the unloading flag
            isUnloading.set(false)
        }
    }

    companion object {
        private class IntVar(value: Int) {
            @Volatile
            var value: Int = value

            fun inc() {
                synchronized(this) {
                    value += 1
                }
            }
        }

        private sealed interface State {
            data object Idle : State
            data class Loaded(val model: Long, val context: Long) : State
        }

        // Enforce only one instance of LLamaAndroid.
        private val _instance: LLamaAndroid = LLamaAndroid()

        fun instance(): LLamaAndroid = _instance
    }
}
