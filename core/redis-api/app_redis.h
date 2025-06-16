/**
 * @file DRDSDataRedis.h
 * @brief
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2024-09-13
 *
 * @copyright Copyright (c) 2024  by  wwk
 *
 * @par 修改日志:单例模式，redis数据库操作接口
 * 该接口不是线程安全的，不建议在多线程环境下使用单个实例操作 <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2024-09-13     <td>1.0     <td>wwk   <td>修改?
 * </table>
 */
#pragma once

#include <hiredis/hiredis.h>
#include <string.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#include "log/logger.h"

#define PIPELINE_SIZE 100
#define REDIS_TIMEOUT 3000

#define FREE_REDIS_REPLY(reply)                                                \
  do {                                                                         \
    if (nullptr != (reply)) {                                                  \
      freeReplyObject(reply);                                                  \
    }                                                                          \
  } while (false)

struct RedisPiplineResult {
  std::vector<char> data;
  bool success;
  int32_t errCode;
};

struct RedisCommandSetError {
  std::string key;           // 错误的键
  std::string error_message; // 错误的原因
};

class DRDSDataRedis {
public:
  static void setDefaultConnectionInfo(const std::string &unix_socket_path = "",
                                       const std::string &host = "",
                                       int port = 0);
  explicit DRDSDataRedis();
  explicit DRDSDataRedis(const std::string &unix_socket_path);
  DRDSDataRedis(const std::string &host, int port);
  ~DRDSDataRedis();
  bool handleConnectionErrors(const std::string &operation);
  bool isKeyExist(const std::string &key);
  template <typename T>
  bool storeValue(const std::string &key, const std::string &format, T value);
  bool storeString(const std::string &key, const std::string &value);
  bool storeInt(const std::string &key, int16_t value);
  bool storeUInt(const std::string &key, uint16_t value);
  bool storeReal(const std::string &key, float value);
  bool storeBool(const std::string &key, bool value);
  bool storeDInt(const std::string &key, int32_t value);
  bool storeUDInt(const std::string &key, uint32_t value);
  bool storeLReal(const std::string &key, double value);
  bool storeSInt(const std::string &key, int8_t value);
  bool storeUSInt(const std::string &key, uint8_t value);
  bool storeLInt(const std::string &key, int64_t value);
  bool storeULInt(const std::string &key, uint64_t value);
  bool storeByte(const std::string &key, uint8_t value);
  bool storeWord(const std::string &key, uint16_t value);
  bool storeDWord(const std::string &key, uint32_t value);
  bool storeLWord(const std::string &key, uint64_t value);
  bool storeChar(const std::string &key, const char *value);
  bool storeBinary(const std::string &key, const char *data, size_t length);
  bool storeBinaryForPipline(const std::string &key, const char *data,
                             size_t length);
  bool processSetRedisReply(const std::string &key,
                            RedisCommandSetError &errors);
  bool getBinary(const std::string &key, std::vector<char> &data);
  int16_t getInt(const std::string &key);
  float getReal(const std::string &key);
  bool getBool(const std::string &key);
  int32_t getDInt(const std::string &key);
  uint32_t getUDInt(const std::string &key);
  double getLReal(const std::string &key);
  int8_t getSInt(const std::string &key);
  uint8_t getUSInt(const std::string &key);
  int64_t getLInt(const std::string &key);
  uint64_t getULInt(const std::string &key);
  std::string getString(const std::string &key);
  uint8_t getByte(const std::string &key);
  uint32_t getDWord(const std::string &key);
  uint64_t getLWord(const std::string &key);
  redisContext *getConnection();

private:
  redisContext *context_ = nullptr;   /*redis 句柄指针*/
  std::string unix_socket_path_ = ""; // 当前使用的 Unix socket 路径
  std::string host_ = "";             // 当前使用的主机名
  int port_;                          // 当前使用的端口号

  static std::string default_unix_socket_path; // 默认 Unix socket 路径
  static std::string default_host;             // 默认主机名
  static int default_port;                     // 默认端口号

private:
  void initializeUnixConnection(const std::string &unix_socket_path);
  // 初始化 TCP 连接
  void initializeTcpConnection(const std::string &host, int port);
  // 检查 Redis 连接
  bool checkRedisConnection(const std::string &connectionType);

  bool reconnectIfNeeded(int timeout_ms = 3000);
};