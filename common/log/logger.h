/**
 * @file config.cc
 * @brief 
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2025-03-28
 * 
 * @copyright Copyright (c) 2025  by  wwk
 * 
 * @par �޸���־:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-03-28     <td>1.0     <td>wwk   <td>�޸�?
 * </table>
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

class Logger {
 public:
  enum severity_level { trace, debug, info, warning, error, critical };
  enum LoggerType { both = 0, console, file };

 public:
  ~Logger();

  /**
   * @brief ��ʼ����־ϵͳ
   *
   * @param fileName        ��־�ļ�����֧�־���·�������·��
   * @param type           ��־���ͣ���ѡֵ��both���ļ��Ϳ���̨��, console������̨��, file���ļ���
   * @param level          ��־���𣬿�����־���������
   * @param maxFileSize    ������־�ļ�������С����λ���ֽڣ�
   * @param maxBackupIndex ������־�ļ����������
   * @param isAsync        �Ƿ������첽��־��Ĭ��Ϊ false��ͬ��ģʽ��
   * @return true          ��ʼ���ɹ�
   * @return false         ��ʼ��ʧ��
   */
  bool Init(
      const std::string& fileName, LoggerType type, severity_level level, int maxFileSize, int maxBackupIndex,
      bool isAsync = false);

  /**
   * @brief ע����־ʵ��
   */
  void Uinit();

  /**
   * @brief Set the Flush Every object  ������־ˢ���ļ���Ƶ�ʣ���λ���� Ĭ��0 ��Ĭ�ϻ���ˢ��
   * @param  flushEvery       ���ʱ��
   */
  void setFlushEvery(uint32_t flushEvery);

  /**
   * @brief Set the Flush On Level object  ������־����ˢ���ļ��ļ��� Ĭ��Ϊerror��������ˢ�� 
   * @param  flushOnLevel     ����
   */
  void setFlushOnLevel(Logger::severity_level flushOnLevel);  

  static Logger& Instance();

 private:
  void Log(severity_level level, const std::string& msg, const char* file, int line, const char* func);
  class LoggerImpl;
  std::unique_ptr<LoggerImpl> pImpl;
  Logger();
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
  friend class LogStream; 
};

class LogStream {
 public:
  LogStream(Logger& logger, Logger::severity_level level, const char* file, int line, const char* func)
      : logger_(logger), level_(level), file_(file), line_(line), func_(func) {}
  ~LogStream() { logger_.Log(level_, stream_.str(), file_, line_, func_); }

  std::ostringstream& stream() { return stream_; }

 private:
  Logger& logger_;
  Logger::severity_level level_;
  const char* file_;
  const char* func_;
  int line_;
  std::ostringstream stream_;

};

class LogRateLimiter {
 public:
  static bool shouldLog(const std::string& key, int interval_ms) {
    using namespace std::chrono;
    thread_local static std::unordered_map<std::string, steady_clock::time_point> last_log_times;

    auto now = steady_clock::now();
    auto it = last_log_times.find(key);
    if (it == last_log_times.end() || duration_cast<milliseconds>(now - it->second).count() >= interval_ms) {
      last_log_times[key] = now;
      return true;
    }
    return false;
  }
};

#define LOG(level) LogStream(Logger::Instance(), Logger::level, __FILE__, __LINE__, __FUNCTION__).stream()
#define LOG_TIME(level, interval_ms) \
  if (LogRateLimiter::shouldLog(std::string(__FILE__) + ":" + std::to_string(__LINE__), interval_ms)) LOG(level)

#endif  // LOGGER_H