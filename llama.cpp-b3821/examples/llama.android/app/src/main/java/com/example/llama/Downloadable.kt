package com.example.llama

import android.app.DownloadManager
import android.net.Uri
import android.util.Log
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import java.io.File
import java.security.MessageDigest

data class Downloadable(
    val name: String,
    val source: Uri,
    val destination: File,
    val sha256: String
) {
    companion object {
        @JvmStatic
        private val tag: String? = this::class.qualifiedName

        sealed interface State
        data object Ready : State
        data class Downloading(val progress: Int, val id: Long) : State
        data class Downloaded(val downloadable: Downloadable) : State
        data class Error(val message: String) : State

        @Composable
        fun DownloadButton(viewModel: MainViewModel, dm: DownloadManager, item: Downloadable) {
            var status by remember {
                mutableStateOf(
                    if (item.destination.exists() && verifyFileSha256(item.destination, item.sha256)) {
                        Downloaded(item)
                    } else {
                        if (item.destination.exists()) {
                            item.destination.delete()
                        }
                        Ready
                    }
                )
            }

            val coroutineScope = rememberCoroutineScope()

            // ダウンロード監視関数
            suspend fun monitorDownload(downloadId: Long): State {
                while (true) {
                    val query = DownloadManager.Query().setFilterById(downloadId)
                    val cursor = dm.query(query)
                    cursor?.use {
                        if (it.moveToFirst()) {
                            val bytesDownloaded = it.getLong(it.getColumnIndexOrThrow(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR))
                            val bytesTotal = it.getLong(it.getColumnIndexOrThrow(DownloadManager.COLUMN_TOTAL_SIZE_BYTES))
                            val progress = if (bytesTotal > 0) ((bytesDownloaded * 100) / bytesTotal).toInt() else 0

                            val statusCode = it.getInt(it.getColumnIndexOrThrow(DownloadManager.COLUMN_STATUS))
                            when (statusCode) {
                                DownloadManager.STATUS_SUCCESSFUL -> {
                                    // ダウンロード完了後にSHA256を検証
                                    if (verifyFileSha256(item.destination, item.sha256)) {
                                        return Downloaded(item)
                                    } else {
                                        item.destination.delete()
                                        return Error("SHA256 checksum mismatch")
                                    }
                                }
                                DownloadManager.STATUS_FAILED -> {
                                    val reason = it.getInt(it.getColumnIndexOrThrow(DownloadManager.COLUMN_REASON))
                                    return Error("Download failed: $reason")
                                }
                                else -> {
                                    // ダウンロード中の進捗を更新
                                    status = Downloading(progress, downloadId)
                                }
                            }
                        } else {
                            return Error("Download not found")
                        }
                    } ?: return Error("Cursor is null")

                    delay(500L)
                }
            }

            fun initiateDownload() {
                item.destination.delete()

                val request = DownloadManager.Request(item.source).apply {
                    setTitle("Downloading model")
                    setDescription("Downloading model: ${item.name}")
                    setAllowedNetworkTypes(
                        DownloadManager.Request.NETWORK_WIFI or DownloadManager.Request.NETWORK_MOBILE
                    )
                    setDestinationUri(Uri.fromFile(item.destination))
                    setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED)
                }

                viewModel.log("Saving ${item.name} to ${item.destination.path}")
                Log.i(tag, "Saving ${item.name} to ${item.destination.path}")

                val downloadId = dm.enqueue(request)
                status = Downloading(0, downloadId)

                coroutineScope.launch {
                    status = monitorDownload(downloadId)
                }
            }

            fun cancelDownload(downloadId: Long) {
                val removed = dm.remove(downloadId)
                if (removed > 0) {
                    viewModel.log("Download cancelled: $downloadId")
                    Log.i(tag, "Download cancelled: $downloadId")
                    status = Ready
                } else {
                    viewModel.log("Failed to cancel download: $downloadId")
                    Log.e(tag, "Failed to cancel download: $downloadId")
                }
            }

            fun onClick() {
                when (status) {
                    is Downloaded -> {
                        viewModel.load(item.destination.path)
                    }
                    is Ready, is Error -> {
                        initiateDownload()
                    }
                    is Downloading -> {
                        // ダウンロード中は何もしない（キャンセルは別ボタン）
                    }
                }
            }

            Button(onClick = { onClick() }, enabled = status !is Downloading) {
                when (val currentStatus = status) {
                    is Downloading -> {
                        Text("Downloading ${currentStatus.progress}%")
                    }
                    is Downloaded -> Text("Load ${item.name}")
                    is Ready -> Text("Download ${item.name}")
                    is Error -> Text("Retry ${item.name}")
                }
            }

            // ダウンロード中の場合にキャンセルボタンを表示
            if (status is Downloading) {
                Spacer(modifier = androidx.compose.ui.Modifier.width(8.dp))
                Button(onClick = {
                    (status as? Downloading)?.let { cancelDownload(it.id) }
                }) {
                    Text("Cancel")
                }
            }
        }

        // SHA256チェックサムを検証する関数
        private fun verifyFileSha256(file: File, expectedSha256: String): Boolean {
            return try {
                val digest = MessageDigest.getInstance("SHA-256")
                file.inputStream().use { fis ->
                    val buffer = ByteArray(1024 * 4)
                    var bytesRead: Int
                    while (fis.read(buffer).also { bytesRead = it } != -1) {
                        digest.update(buffer, 0, bytesRead)
                    }
                }
                val actualSha256 = digest.digest().joinToString("") { "%02x".format(it) }
                actualSha256.equals(expectedSha256, ignoreCase = true)
            } catch (e: Exception) {
                Log.e(tag, "Failed to compute SHA256: ${e.message}")
                false
            }
        }
    }
}
