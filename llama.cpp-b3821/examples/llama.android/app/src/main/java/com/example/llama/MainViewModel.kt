// llama.cpp-b3821/examples/llama.android/app/src/main/java/com/example/llama/MainViewModel.kt

package com.example.llama

import android.llama.cpp.LLamaAndroid
import android.util.Log
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.launch

class MainViewModel(private val llamaAndroid: LLamaAndroid = LLamaAndroid.instance()) : ViewModel() {
    companion object {
        @JvmStatic
        private val NanosPerSecond = 1_000_000_000.0
    }

    private val tag: String? = this::class.simpleName

    var messages by mutableStateOf(listOf(ChatMessage(Sender.LLM, "Initializing...")))
        private set

    var message by mutableStateOf("")
        private set

    var showMemoryInfo by mutableStateOf(false)
        private set

    var showModelPath by mutableStateOf(false)
        private set

    var currentModelPath: String? = null
        private set

    var isLoading by mutableStateOf(false)
        private set

    private var sendJob: Job? = null

    override fun onCleared() {
        super.onCleared()

        viewModelScope.launch {
            try {
                llamaAndroid.unload()
            } catch (exc: IllegalStateException) {
                messages += ChatMessage(Sender.LLM, exc.message!!)
            }
        }
    }

    fun send() {
        val text = message.trim()
        if (text.isEmpty()) return // 空メッセージは無視
        message = ""

        // ユーザーのメッセージを追加
        messages = messages + ChatMessage(Sender.User, text)

        sendJob = viewModelScope.launch {
            val responseBuilder = StringBuilder()
            try {
                // まず空のLLMメッセージを追加
                val initialMessage = ChatMessage(Sender.LLM, "")
                messages = messages + initialMessage
                val llmMessageIndex = messages.size - 1

                llamaAndroid.send(text)
                    .catch { e ->
                        Log.e(tag, "send() failed", e)
                        messages = messages.toMutableList().apply {
                            set(llmMessageIndex, ChatMessage(Sender.LLM, e.message ?: "Unknown error"))
                        }
                    }
                    .collect { token ->
                        responseBuilder.append(token)
                        // リアルタイムでメッセージを更新
                        messages = messages.toMutableList().apply {
                            set(llmMessageIndex, ChatMessage(Sender.LLM, responseBuilder.toString()))
                        }
                    }
            } catch (e: CancellationException) {
                Log.i(tag, "send() canceled")
                messages = messages + ChatMessage(Sender.LLM, "Operation canceled.")
            } catch (e: Exception) {
                Log.e(tag, "send() failed", e)
                messages = messages + ChatMessage(Sender.LLM, "Error: ${e.message ?: "Unknown error"}")
            }
        }
    }

    private fun cancelSend() {
        sendJob?.cancel()
    }

    fun bench(pp: Int, tg: Int, pl: Int, nr: Int = 1) {
        viewModelScope.launch {
            try {
                val start = System.nanoTime()
                val warmupResult = llamaAndroid.bench(pp, tg, pl, nr)
                val end = System.nanoTime()

                messages += ChatMessage(Sender.LLM, warmupResult)

                val warmup = (end - start).toDouble() / NanosPerSecond
                messages += ChatMessage(Sender.LLM, "Warm up time: $warmup seconds, please wait...")

                if (warmup > 5.0) {
                    messages += ChatMessage(Sender.LLM, "Warm up took too long, aborting benchmark")
                    return@launch
                }

                messages += ChatMessage(Sender.LLM, llamaAndroid.bench(512, 128, 1, 3))
            } catch (exc: IllegalStateException) {
                Log.e(tag, "bench() failed", exc)
                messages += ChatMessage(Sender.LLM, exc.message!!)
            }
        }
    }

    fun load(pathToModel: String) {
        if (isLoading) {
            messages += ChatMessage(Sender.LLM, "Model is already loading. Please wait.")
            return
        }

        isLoading = true
        viewModelScope.launch {
            try {
                // 送信中の操作をキャンセル
                cancelSend()

                llamaAndroid.load(pathToModel)
                currentModelPath = pathToModel
                messages += ChatMessage(Sender.LLM, "Loaded $pathToModel")
            } catch (exc: IllegalStateException) {
                Log.e(tag, "load() failed", exc)
                messages += ChatMessage(Sender.LLM, exc.message!!)
            } finally {
                isLoading = false
            }
        }
    }

    fun updateMessage(newMessage: String) {
        message = newMessage
    }

    fun clear() {
        messages = listOf()
    }

    fun log(message: String) {
        messages += ChatMessage(Sender.LLM, message)
    }

    fun toggleMemoryInfo() {
        showMemoryInfo = !showMemoryInfo
    }

    fun toggleModelPath() {
        showModelPath = !showModelPath
    }
}
