#pragma once
#include <QImage>
#include <QObject>
#include <QTimer>
#include <functional>
#include <vector>

namespace s21 {

class FrameProvider {
 public:
  virtual ~FrameProvider() = default;
  virtual QImage captureFrame() const = 0;
};

class GifRecorder : public QObject {
  Q_OBJECT
 public:
  explicit GifRecorder(QObject* parent = nullptr);

  // Настройки записи
  void setFrameProvider(FrameProvider* provider);
  void setOutputSize(const QSize& size);
  void setFrameCount(int count);
  void setFps(int fps);

  // Управление
  bool isRecording() const;
  void startRecording();
  void stopRecording();

  bool saveToFile(const QString& filepath);

 signals:
  void progress(int percent);
  void finished();
  void error(const QString& message);

 private slots:
  void onTimerTick();

 private:
  QTimer* m_timer = nullptr;
  FrameProvider* m_frameProvider = nullptr;

  QSize m_outputSize{640, 480};
  int m_targetFrameCount = 50;
  int m_fps = 10;

  std::vector<QImage> m_buffer;
  int m_capturedFrames = 0;
  bool m_recording = false;
};

}  // namespace s21