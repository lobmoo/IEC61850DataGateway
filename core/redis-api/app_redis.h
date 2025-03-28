/**
 * @file DRDSDataRedis.h
 * @brief
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2024-09-13
 *
 * @copyright Copyright (c) 2024  by  wwk
 *
 * @par �޸���־:����ģʽ��redis���ݿ�����ӿ�
 * �ýӿڲ����̰߳�ȫ�ģ��������ڶ��̻߳�����ʹ�õ���ʵ������ <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2024-09-13     <td>1.0     <td>wwk   <td>�޸�?
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
  std::string key;           // ����ļ�
  std::string error_message; // �����ԭ��
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
  redisContext *context_ = nullptr;   /*redis ���ָ��*/
  std::string unix_socket_path_ = ""; // ��ǰʹ�õ� Unix socket ·��
  std::string host_ = "";             // ��ǰʹ�õ�������
  int port_;                          // ��ǰʹ�õĶ˿ں�

  static std::string default_unix_socket_path; // Ĭ�� Unix socket ·��
  static std::string default_host;             // Ĭ��������
  static int default_port;                     // Ĭ�϶˿ں�

private:
  void initializeUnixConnection(const std::string &unix_socket_path);
  // ��ʼ�� TCP ����
  void initializeTcpConnection(const std::string &host, int port);
  // ��� Redis ����
  bool checkRedisConnection(const std::string &connectionType);

  bool reconnectIfNeeded(int timeout_ms = 3000);
};