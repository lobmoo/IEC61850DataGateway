/**
 * @file DRDSDataRedis.h
 * @brief
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2024-09-13
 *
 * @copyright Copyright (c) 2024  by  wwk
 *
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2024-09-13     <td>1.0     <td>wwk   <td>修改?
 * </table>
 */
#include "app_redis.h"

#include <hiredis/hiredis.h>

std::string DRDSDataRedis::default_unix_socket_path = "";
std::string DRDSDataRedis::default_host = "";
int DRDSDataRedis::default_port = 0;

void DRDSDataRedis::setDefaultConnectionInfo(
    const std::string &unix_socket_path, const std::string &host, int port) {
  default_unix_socket_path = unix_socket_path;
  default_host = host;
  default_port = port;
  LOG(info) << "Default connection info set: unix_socket_path="
                << unix_socket_path << ", host=" << host << ", port=" << port;
}

DRDSDataRedis::DRDSDataRedis() {
  if (!default_unix_socket_path.empty()) {
    initializeUnixConnection(default_unix_socket_path);
  } else if (!default_host.empty() && default_port > 0) {
    initializeTcpConnection(default_host, default_port);
  } else {
    LOG(critical) << "Default connection info is not set";
  }
}

DRDSDataRedis::DRDSDataRedis(const std::string &unix_socket_path) {
  initializeUnixConnection(unix_socket_path);
}

DRDSDataRedis::DRDSDataRedis(const std::string &host, int port) {
  initializeTcpConnection(host, port);
}

DRDSDataRedis::~DRDSDataRedis() {
  if (context_) {
    redisFree(context_);
  }
}

bool DRDSDataRedis::handleConnectionErrors(const std::string &operation) {
  if (context_ == nullptr || context_->err) {
    LOG(error) << "Redis error during operation '" << operation
                   << "': " << (context_ ? context_->errstr : "Unknown error");
    return reconnectIfNeeded();
  }
  return true;
}

bool DRDSDataRedis::isKeyExist(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "EXISTS %s", key.c_str());
  if (reply == nullptr) {
    LOG(error) << "Redis error during operation 'EXISTS':"
                   << (context_ ? context_->errstr : "Unknown error");

    return false;
  }

  bool bFlag = false;

  if (reply->type == REDIS_REPLY_INTEGER) {
    if (reply->integer == 1) {
      bFlag = true;
    }
  }

  FREE_REDIS_REPLY(reply);
  return bFlag;
}

template <typename T>
bool DRDSDataRedis::storeValue(const std::string &key,
                               const std::string &format, T value) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, format.c_str(), key.c_str(), value);
  if (reply == nullptr) {
    if (context_ && context_->err) {
      std::cerr << "Redis command error: " << context_->errstr << std::endl;
    }
    return handleConnectionErrors("SET");
  }

  if (reply->type == REDIS_REPLY_ERROR) {
    std::cerr << "Redis reply error: " << reply->str << std::endl;
    FREE_REDIS_REPLY(reply);
    return false;
  }

  FREE_REDIS_REPLY(reply);
  return true;
}

bool DRDSDataRedis::storeString(const std::string &key,
                                const std::string &value) {
  return storeValue(key, "SET %s %s", value.c_str());
}

bool DRDSDataRedis::storeInt(const std::string &key, int16_t value) {
  return storeValue(key, "SET %s %d", value);
}

bool DRDSDataRedis::storeUInt(const std::string &key, uint16_t value) {
  return storeValue(key, "SET %s %u", value);
}

bool DRDSDataRedis::storeReal(const std::string &key, float value) {
  return storeValue(key, "SET %s %f", value);
}

bool DRDSDataRedis::storeBool(const std::string &key, bool value) {
  return storeValue(key, "SET %s %d", value ? 1 : 0);
}

bool DRDSDataRedis::storeDInt(const std::string &key, int32_t value) {
  return storeValue(key, "SET %s %d", value);
}

bool DRDSDataRedis::storeUDInt(const std::string &key, uint32_t value) {
  return storeValue(key, "SET %s %u", value);
}

bool DRDSDataRedis::storeLReal(const std::string &key, double value) {
  return storeValue(key, "SET %s %f", value);
}

bool DRDSDataRedis::storeSInt(const std::string &key, int8_t value) {
  return storeValue(key, "SET %s %d", value);
}

bool DRDSDataRedis::storeUSInt(const std::string &key, uint8_t value) {
  return storeValue(key, "SET %s %u", value);
}

bool DRDSDataRedis::storeLInt(const std::string &key, int64_t value) {
  return storeValue(key, "SET %s %lld", value);
}

bool DRDSDataRedis::storeULInt(const std::string &key, uint64_t value) {
  return storeValue(key, "SET %s %llu", value);
}

bool DRDSDataRedis::storeByte(const std::string &key, uint8_t value) {
  return storeValue(key, "SET %s %u", value);
}

bool DRDSDataRedis::storeWord(const std::string &key, uint16_t value) {
  return storeValue(key, "SET %s %u", value);
}

bool DRDSDataRedis::storeDWord(const std::string &key, uint32_t value) {
  return storeValue(key, "SET %s %u", value);
}

bool DRDSDataRedis::storeLWord(const std::string &key, uint64_t value) {
  return storeValue(key, "SET %s %llu", value);
}

bool DRDSDataRedis::storeChar(const std::string &key, const char *value) {
  if (!value) {
    LOG(error) << "storeChar failed: value is nullptr.";
    return false;
  }
  return storeValue(key, "SET %s %s", value);
}

bool DRDSDataRedis::storeBinary(const std::string &key, const char *data,
                                size_t length) {
  if (!data || length == 0) {
    LOG(error) << "storeBinary failed: data is nullptr or length is 0.";
    return false;
  }

  redisReply *reply = (redisReply *)redisCommand(context_, "SET %s %b",
                                                 key.c_str(), data, length);
  if (reply == nullptr) {
    if (context_ && context_->err) {
      LOG(error) << "Redis command error: " << context_->errstr;
    }
    return handleConnectionErrors("SET");
  }

  if (reply->type == REDIS_REPLY_ERROR) {
    LOG(error) << "Redis reply error: " << reply->str;
    FREE_REDIS_REPLY(reply);
    return false;
  }

  FREE_REDIS_REPLY(reply);
  return true;
}

bool DRDSDataRedis::storeBinaryForPipline(const std::string &key,
                                          const char *data, size_t length) {
  return redisAppendCommand(context_, "SET %b %b", key.c_str(), key.length(),
                            data, length) == REDIS_OK
             ? true
             : false;
}


bool DRDSDataRedis::processSetRedisReply(const std::string &key,
                                         RedisCommandSetError &errors) {
  redisReply *reply = nullptr;
  bool success = true;

  if (redisGetReply(context_, (void **)&reply) == REDIS_OK) {
    if (reply != nullptr) {
      if (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0) {
        // 正常，不需要额外操作
      } else {
        LOG(error) << "Failed to execute SET command for key: " << key
                       << ", error: " << (reply->str ? reply->str : "NULL");
        success = false;
        errors = {key, "Failed to execute SET command"};
      }
      FREE_REDIS_REPLY(reply);
    } else {
      LOG(error) << "Received NULL reply from Redis for key: " << key;
      success = false;
      errors = {key, "Received NULL reply"};
    }
  } else {
    LOG(error) << "Error receiving reply from Redis for key: " << key;
    success = false;
    errors = {key, "Error receiving reply"};
  }

  return success;
}


bool DRDSDataRedis::getBinary(const std::string &key, std::vector<char> &data) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    return handleConnectionErrors("GET");
  }

  if (reply->type == REDIS_REPLY_STRING && reply->str != nullptr) {
    data.assign(reply->str,
                reply->str + reply->len);  // 自动调整大小并拷贝数据
  } else {
    LOG(error) << "Unexpected reply type: " << reply->type;
    FREE_REDIS_REPLY(reply);
    return false;
  }

  FREE_REDIS_REPLY(reply);
  return true;
}


bool DRDSDataRedis::getBool(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    handleConnectionErrors("GET");
    return false;
  }

  bool value = std::stoi(reply->str) != 0;
  FREE_REDIS_REPLY(reply);
  return value;
}

int32_t DRDSDataRedis::getDInt(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    handleConnectionErrors("GET");
    return 0;
  }

  int32_t value = static_cast<int32_t>(std::stol(reply->str));
  FREE_REDIS_REPLY(reply);
  return value;
}

uint32_t DRDSDataRedis::getUDInt(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    handleConnectionErrors("GET");
    return 0;
  }

  uint32_t value = static_cast<uint32_t>(std::stoul(reply->str));
  FREE_REDIS_REPLY(reply);
  return value;
}

double DRDSDataRedis::getLReal(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    handleConnectionErrors("GET");
    return 0.0;
  }

  double value = std::stod(reply->str);
  FREE_REDIS_REPLY(reply);
  return value;
}

int8_t DRDSDataRedis::getSInt(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    handleConnectionErrors("GET");
    return 0;
  }

  int8_t value = static_cast<int8_t>(std::stoi(reply->str));
  FREE_REDIS_REPLY(reply);
  return value;
}

uint8_t DRDSDataRedis::getUSInt(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    handleConnectionErrors("GET");
    return 0;
  }

  uint8_t value = static_cast<uint8_t>(std::stoul(reply->str));
  FREE_REDIS_REPLY(reply);
  return value;
}

int64_t DRDSDataRedis::getLInt(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    handleConnectionErrors("GET");
    return 0;
  }

  int64_t value = static_cast<int64_t>(std::stoll(reply->str));
  FREE_REDIS_REPLY(reply);
  return value;
}

uint64_t DRDSDataRedis::getULInt(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    handleConnectionErrors("GET");
    return 0;
  }

  uint64_t value = static_cast<uint64_t>(std::stoull(reply->str));
  FREE_REDIS_REPLY(reply);
  return value;
}

std::string DRDSDataRedis::getString(const std::string &key) {
  redisReply *reply =
      (redisReply *)redisCommand(context_, "GET %s", key.c_str());
  if (reply == nullptr || context_->err) {
    handleConnectionErrors("GET");
    return "";
  }

  std::string value = reply->str;
  FREE_REDIS_REPLY(reply);
  return value;
}

uint8_t DRDSDataRedis::getByte(const std::string &key) { return getUSInt(key); }

uint32_t DRDSDataRedis::getDWord(const std::string &key) {
  return getUDInt(key);
}

uint64_t DRDSDataRedis::getLWord(const std::string &key) {
  return getULInt(key);
}

redisContext *DRDSDataRedis::getConnection() { return context_; }

void DRDSDataRedis::initializeUnixConnection(
    const std::string &unix_socket_path) {
  struct timeval timeout;
  timeout.tv_sec = REDIS_TIMEOUT / 1000;
  timeout.tv_usec = (REDIS_TIMEOUT % 1000) * 1000;
  unix_socket_path_ = unix_socket_path;
  context_ = redisConnectUnixWithTimeout(unix_socket_path.c_str(), timeout);
  checkRedisConnection("Unix socket connection");
}

// 初始化 TCP 连接
void DRDSDataRedis::initializeTcpConnection(const std::string &host, int port) {
  struct timeval timeout;
  timeout.tv_sec = REDIS_TIMEOUT / 1000;
  timeout.tv_usec = (REDIS_TIMEOUT % 1000) * 1000;
  host_ = host;
  port_ = port;
  context_ = redisConnectWithTimeout(host.c_str(), port, timeout);
  checkRedisConnection("TCP connection");
}

// 检查 Redis 连接
bool DRDSDataRedis::checkRedisConnection(const std::string &connectionType) {
  if (context_ == nullptr || context_->err) {
    LOG(error) << connectionType << " failed: "
                   << (context_ ? context_->errstr
                                : "Can't allocate redis context_");
    if (context_) {
      redisFree(context_);
    }

    LOG(error)
        << "Failed to connect to Redis, please check your redis Server";
    throw std::runtime_error("Invalid default Redis connection info");
    return false;
  }
  return true;
}

bool DRDSDataRedis::reconnectIfNeeded(int timeout_ms) {
  if (context_ != nullptr) {
    redisFree(context_);
  }

  struct timeval timeout;
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;

  // 根据当前配置重新连接
  if (!unix_socket_path_.empty()) {
    context_ = redisConnectUnixWithTimeout(unix_socket_path_.c_str(), timeout);
  } else {
    context_ = redisConnectWithTimeout(host_.c_str(), port_, timeout);
  }

  if (context_ == nullptr || context_->err) {
    LOG(error) << "Error: "
                   << (context_ ? context_->errstr
                                : "Can't allocate redis context_");
    if (context_) {
      redisFree(context_);
    }
    LOG(critical) << "Redis reconnection failed or timed out";
  }
  return true;
}
