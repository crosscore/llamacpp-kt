// llama.cpp-b3821/examples/llama.android/app/src/main/java/com/example/llama/MainActivity.kt

package com.example.llama

import android.app.ActivityManager
import android.app.DownloadManager
import android.content.ClipData
import android.content.ClipboardManager
import android.net.Uri
import android.os.Bundle
import android.os.StrictMode
import android.os.StrictMode.VmPolicy
import android.text.format.Formatter
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.getSystemService
import com.example.llama.ui.theme.LlamaAndroidTheme
import java.io.File

class MainActivity(
    activityManager: ActivityManager? = null,
    downloadManager: DownloadManager? = null,
    clipboardManager: ClipboardManager? = null,
) : ComponentActivity() {

    private val activityManager by lazy { activityManager ?: getSystemService<ActivityManager>()!! }
    private val downloadManager by lazy { downloadManager ?: getSystemService<DownloadManager>()!! }
    private val clipboardManager by lazy { clipboardManager ?: getSystemService<ClipboardManager>()!! }

    private val viewModel: MainViewModel by viewModels()

    private fun availableMemory(): ActivityManager.MemoryInfo {
        return ActivityManager.MemoryInfo().also { memoryInfo ->
            activityManager.getMemoryInfo(memoryInfo)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        StrictMode.setVmPolicy(
            VmPolicy.Builder(StrictMode.getVmPolicy())
                .detectLeakedClosableObjects()
                .build()
        )

        val free = Formatter.formatFileSize(this, availableMemory().availMem)
        val total = Formatter.formatFileSize(this, availableMemory().totalMem)

        viewModel.log("Current memory: $free / $total")
        viewModel.log("Downloads directory: ${getExternalFilesDir(null)}")

        val extFilesDir = getExternalFilesDir(null)

        val models = listOf(
            Downloadable(
                "Qwen2.5 1.5B (Q8_0, 1.65 GiB)",
                Uri.parse("https://huggingface.co/QuantFactory/Qwen2.5-1.5B-GGUF/resolve/main/Qwen2.5-1.5B.Q8_0.gguf?download=true"),
                File(extFilesDir, "qwen-2.5-1.5B-q8_0.gguf"),
                sha256 = "a7d1fe4d251eb42b531d169551ebd0befc83c85f304f13825cba5e65e7dba330"
            ),
            Downloadable(
                "Qwen2.5 1.5B (Q2_K, 0.67 GiB)",
                Uri.parse("https://huggingface.co/QuantFactory/Qwen2.5-1.5B-GGUF/resolve/main/Qwen2.5-1.5B.Q2_K.gguf?download=true"),
                File(extFilesDir, "qwen-2.5-1.5B-Q2_K.gguf"),
                sha256 = "081813878c024756d5069a9477108fde65e47d151717ff5e6dc608abd1645f03"
            ),
        )

        setContent {
            LlamaAndroidTheme {
                // A surface container using the 'background' color from the theme
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    MainCompose(
                        viewModel,
                        clipboardManager,
                        downloadManager,
                        models,
                    )
                }

            }
        }
    }
}

@Composable
fun MainCompose(
    viewModel: MainViewModel,
    clipboard: ClipboardManager,
    dm: DownloadManager,
    models: List<Downloadable>
) {
    val context = LocalContext.current

    Column(modifier = Modifier.padding(16.dp)) {
        // メモリ情報の表示
        if (viewModel.showMemoryInfo) {
            val activityManager = context.getSystemService<ActivityManager>()
            val memoryInfo = ActivityManager.MemoryInfo()
            activityManager?.getMemoryInfo(memoryInfo)
            val free = Formatter.formatFileSize(context, memoryInfo.availMem)
            val total = Formatter.formatFileSize(context, memoryInfo.totalMem)
            Text("Memory: $free / $total", modifier = Modifier.padding(bottom = 8.dp))
        }

        // モデルパスの表示
        if (viewModel.showModelPath) {
            viewModel.currentModelPath?.let {
                Text("Model Path: $it", modifier = Modifier.padding(bottom = 8.dp))
            }
        }

        val scrollState = rememberLazyListState()

        Box(modifier = Modifier.weight(1f)) {
            LazyColumn(
                state = scrollState,
                modifier = Modifier.fillMaxSize()
            ) {
                items(viewModel.messages) { chatMessage ->
                    ChatMessageView(chatMessage)
                }
            }
        }
        OutlinedTextField(
            value = viewModel.message,
            onValueChange = { viewModel.updateMessage(it) },
            label = { Text("Message") },
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 8.dp),
            enabled = !viewModel.isLoading // 入力を無効化
        )

        // 1つ目のボタン行: Send, Clear, Copy
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 4.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = { viewModel.send() },
                modifier = Modifier.weight(1f),
                enabled = !viewModel.isLoading // ロード中は無効化
            ) { Text("Send") }
            Button(
                onClick = { viewModel.clear() },
                modifier = Modifier.weight(1f),
                enabled = !viewModel.isLoading // ロード中は無効化
            ) { Text("Clear") }
            Button(
                onClick = {
                    viewModel.messages.joinToString("\n") { message ->
                        when (message.sender) {
                            Sender.User -> "User: ${message.content}"
                            Sender.LLM -> "LLM: ${message.content}"
                        }
                    }.let {
                        clipboard.setPrimaryClip(ClipData.newPlainText("", it))
                    }
                },
                modifier = Modifier.weight(1f)
            ) { Text("Copy") }
        }

        // 2つ目のボタン行: Memory, Model Path
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 4.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = { viewModel.toggleMemoryInfo() },
                modifier = Modifier.weight(1f)
            ) { Text("Memory") }
            Button(
                onClick = { viewModel.toggleModelPath() },
                modifier = Modifier.weight(1f)
            ) { Text("Model Path") }
        }

        // モデルのダウンロードボタン
        Column(modifier = Modifier.padding(top = 8.dp)) {
            for (model in models) {
                Downloadable.DownloadButton(viewModel, dm, model, viewModel.isLoading)
            }
        }

        // ロード中にプログレスインジケーターを表示
        if (viewModel.isLoading) {
            LinearProgressIndicator(modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 8.dp))
        }
    }
}

@Composable
fun ChatMessageView(chatMessage: ChatMessage) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp)
    ) {
        Text(
            text = when (chatMessage.sender) {
                Sender.User -> "User: "
                Sender.LLM -> "LLM: "
            },
            style = MaterialTheme.typography.bodyMedium,
            color = when (chatMessage.sender) {
                Sender.User -> MaterialTheme.colorScheme.primary
                Sender.LLM -> MaterialTheme.colorScheme.secondary
            },
            modifier = Modifier.width(60.dp) // ラベルの幅を固定
        )
        Text(
            text = chatMessage.content,
            style = MaterialTheme.typography.bodyLarge,
            color = LocalContentColor.current,
            modifier = Modifier
                .weight(1f)
        )
    }
}
