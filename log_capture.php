<?php
// WDM2VST Telemetry Logger
// 允许跨域（如果有网页端测试的需求）
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Headers: Content-Type");

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    die("Method Not Allowed");
}

// 获取原始 JSON 数据
$jsonData = file_get_contents('php://input');
$data = json_decode($jsonData, true);

if ($data === null) {
    http_response_code(400);
    die("Bad Request: Invalid JSON");
}

// 安全提取字段
$event = isset($data['event']) ? $data['event'] : 'UnknownEvent';
$details = isset($data['details']) ? $data['details'] : '';
$plugin = isset($data['plugin']) ? $data['plugin'] : 'UnknownPlugin';
$os = isset($data['os']) ? $data['os'] : 'UnknownOS';
$device = isset($data['device']) ? $data['device'] : 'UnknownDevice';
$time = isset($data['time']) ? $data['time'] : date('Y-m-d\TH:i:s');
$ip = $_SERVER['REMOTE_ADDR'];

// 构建日志条目格式
$logEntry = sprintf("[%s] IP:%s | OS:%s | Player:%s | Plugin:%s | Event:%s | Details:%s\n",
    $time, $ip, $os, $device, $plugin, $event, $details
);

// 按天分割日志文件（例如：logs_wdm2vst/2026-03-26.log）
$logDir = __DIR__ . '/logs_wdm2vst';
$logFileName = date('Y-m-d') . '.log';
$logFilePath = $logDir . '/' . $logFileName;

// 创建安全目录
if (!is_dir($logDir)) {
    mkdir($logDir, 0755, true);
    // 保护日志不允许别人通过网址直接下载
    file_put_contents($logDir . '/.htaccess', "Require all denied");
}

// 追加写入日志，使用排他锁防止并发爆音
file_put_contents($logFilePath, $logEntry, FILE_APPEND | LOCK_EX);

// 成功响应
header('Content-Type: application/json');
echo json_encode(['status' => 'success']);
?>
