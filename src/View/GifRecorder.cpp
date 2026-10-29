#include "GifRecorder.h"

#include <gif.h>

namespace s21 {

GifRecorder::GifRecorder(QObject* parent)
    : QObject(parent), m_timer(new QTimer(this)) {
  connect(m_timer, &QTimer::timeout, this, &GifRecorder::onTimerTick);
}

void GifRecorder::setFrameProvider(FrameProvider* provider) {
  m_frameProvider = provider;
}

void GifRecorder::setOutputSize(const QSize& size) { m_outputSize = size; }

void GifRecorder::setFrameCount(int count) { m_targetFrameCount = count; }

void GifRecorder::setFps(int fps) { m_fps = fps; }

bool GifRecorder::isRecording() const { return m_recording; }

void GifRecorder::startRecording() {
  if (!m_frameProvider) {
    emit error("Frame provider not set");
    return;
  }
  if (m_recording) return;

  m_buffer.clear();
  m_buffer.reserve(m_targetFrameCount);
  m_capturedFrames = 0;
  m_recording = true;

  int intervalMs = 1000 / m_fps;
  m_timer->start(intervalMs);
}

void GifRecorder::stopRecording() {
  if (!m_recording) return;

  m_timer->stop();
  m_recording = false;
  emit finished();
}

void GifRecorder::onTimerTick() {
  if (m_capturedFrames >= m_targetFrameCount) {
    stopRecording();
    return;
  }

  QImage frame = m_frameProvider->captureFrame();
  if (!frame.isNull()) {
    m_buffer.push_back(frame.scaled(m_outputSize, Qt::IgnoreAspectRatio,
                                    Qt::SmoothTransformation));
    m_capturedFrames++;
    emit progress(m_capturedFrames * 100 / m_targetFrameCount);
  }
}

bool GifRecorder::saveToFile(const QString& filepath) {
  if (m_buffer.empty()) {
    emit error("No frames to save");
    return false;
  }

  QString finalPath = filepath;
  if (!finalPath.endsWith(".gif")) {
    finalPath += ".gif";
  }

  GifWriter writer;
  int delay = 100 / m_fps;  // сотые доли секунды

  if (!GifBegin(&writer, finalPath.toStdString().c_str(), m_outputSize.width(),
                m_outputSize.height(), delay)) {
    emit error("Failed to initialize GIF writer");
    return false;
  }

  for (const auto& img : m_buffer) {
    auto rgba = img.convertToFormat(QImage::Format_RGBA8888);
    GifWriteFrame(&writer, rgba.bits(), m_outputSize.width(),
                  m_outputSize.height(), delay);
  }

  GifEnd(&writer);
  m_buffer.clear();
  return true;
}

}  // namespace s21