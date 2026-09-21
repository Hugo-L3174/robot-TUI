#pragma once

#define INST_PING 0x01
#define INST_READ 0x02
#define INST_WRITE 0x03
#define INST_REG_WRITE 0x04
#define INST_REG_ACTION 0x05
#define INST_SYNC_READ 0x82
#define INST_SYNC_WRITE 0x83

#include "servo_types.h"
#include <serial.h>

namespace feetech_servo
{

std::string getModelType(uint16_t id);

enum ModelSeries
{
  SMCL,
  SMBL,
  STS,
  SCS
};

ModelSeries getModelSeries(std::string modelName);

class SCSerial
{
public:
  SCSerial(serial_t * serialPeriph);

  void set_end(uint8_t end)
  {
    end_ = end;
  }

  // Write data normally
  int gen_write(uint8_t id, uint8_t mem_addr, uint8_t * n_dat, uint8_t n_len);
  // Write data in a buffer and wait for action signal to trigger it
  int reg_write(uint8_t id, uint8_t mem_addr, uint8_t * n_dat, uint8_t n_len);
  // Trigger buffered data write (broadcast by default)
  int reg_write_action(uint8_t id = 0xfe);
  // Write sychronously to all ids with each their data
  void sync_write(uint8_t id[], uint8_t idn, uint8_t mem_addr, uint8_t * n_dat, uint8_t n_len);
  int write_byte(uint8_t id, uint8_t mem_addr, uint8_t b_dat);
  int write_word(uint8_t id, uint8_t mem_addr, uint16_t w_dat);
  int read(uint8_t id, uint8_t mem_addr, uint8_t * n_data, uint8_t n_len);
  int read_byte(uint8_t id, uint8_t mem_addr);
  int read_word(uint8_t id, uint8_t mem_addr);
  int ping(uint8_t id);
  int write(uint8_t * n_dat, int n_len);
  int read(uint8_t * n_dat, int n_len);
  int write(uint8_t b_dat);
  void read_flush()
  { /* do not anything */
  }
  void write_flush()
  { /* do not anything */
  }
  int read_model_number(int id);

  void set_timeout(uint16_t timeout)
  {
    timeout_ = timeout;
  }

  void write_buf(uint8_t id, uint8_t mem_addr, uint8_t * n_dat, uint8_t n_len, uint8_t fun);
  /**
   * @brief host to scs format: split a 16bit value to two 8bit values to handle endianness
   *
   * @param data_l ptr to store low byte
   * @param data_h ptr to store high byte
   * @param data data to be split
   */
  void host_2_scs(uint8_t * data_l, uint8_t * data_h, uint16_t data);
  /**
   * @brief scs to host format: combine two 8bit vals into one 16bit
   *
   * @param data_l low byte
   * @param data_h high byte
   * @return uint16_t value
   */
  uint16_t scs_2_host(uint8_t data_l, uint8_t data_h);
  int ask(uint8_t id);
  int check_head();

protected:
  // Servo return level
  uint8_t level_;
  // Is it big endian or little endian?
  uint8_t end_;
  // Servo status Rudder status
  uint8_t error_;
  uint8_t sync_read_rx_packet_index_;
  uint8_t sync_read_rx_packet_len_;
  uint8_t * sync_read_rx_packet_;

  uint16_t timeout_ = 50;

  serial_t * serialPeriph_;
};

} // namespace feetech_servo

#include "scscl.h"
#include "sms_sts.h"
