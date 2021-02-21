#include "power.h"
#include "jade_assert.h"
#include <driver/i2c.h>

#define I2C_BATTERY_PORT I2C_NUM_0

#define ACK_CHECK_EN 0x1 /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0 /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0 /*!< I2C ack value */
#define NACK_VAL 0x1 /*!< I2C nack value */

#if 0
#define I2C_CHECK_RET(expr)                                                                                            \
    do {                                                                                                               \
        const esp_err_t res = (expr);                                                                                  \
        if (res != ESP_OK) {                                                                                           \
            JADE_LOGE("i2c call returned: %u", res);                                                                   \
            return res;                                                                                                \
        }                                                                                                              \
    } while (false)
#define I2C_LOG_ANY_ERROR(expr)                                                                                        \
    do {                                                                                                               \
        const esp_err_t res = (expr);                                                                                  \
        if (res != ESP_OK) {                                                                                           \
            JADE_LOGE("i2c call returned: %u", res);                                                                   \
        }                                                                                                              \
    } while (false)
#endif

#define I2C_CHECK_RET(expr)                                                                                            \
    do {                                                                                                               \
        const esp_err_t res = (expr);                                                                                  \
        if (res != ESP_OK) {                                                                                           \
            vTaskDelay(1);                                                                                             \
            continue;                                                                                                  \
        }                                                                                                              \
    } while (false)
#define I2C_LOG_ANY_ERROR(expr)                                                                                        \
    do {                                                                                                               \
        const esp_err_t res = (expr);                                                                                  \
        if (res != ESP_OK) {                                                                                           \
            vTaskDelay(1);                                                                                             \
            continue;                                                                                                  \
        }                                                                                                              \
    } while (false)


static esp_err_t write_command(uint8_t reg, uint8_t val);
static bool led;

esp_err_t power_init()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 21;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = 22;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;

    I2C_CHECK_RET(i2c_param_config(I2C_BATTERY_PORT, &conf));
    I2C_CHECK_RET(i2c_driver_install(I2C_BATTERY_PORT, conf.mode, 0, 0, 0));

    // LDO2/3
    //      LDO2: 3300 ==> (3300 - 1800)/100 ==> 15
    write_command(0x28, 15 << 4);
    // DCDC3: back light:28-140
    //      DC3: 2700 ==> (2700 - 700)/25 ==> 80
    write_command(0x27, 80);
    // Voff
    //      3000 ==> (3000 - 2600)/100 ==> 4
    write_command(0x31, 4);
    // Charge
    //      100mA    [3:0]=0000
    //               [4]  =0
    //      4.2V     [6:5]=10
    //      enable   [7]  =1
    write_command(0x33, 0xc0);
    // PEK
    //      startup 128ms   [7:6]=00
    //                      [5:2]=0111
    //      power off 4s    [1:0]=00
    write_command(0x36, 0x1c);
    // enable
    //              [7:3]=00000
    //      LDO2    [2]=1
    //      DCDC3   [1]=1
    //      DCDC1   [0]=1
    write_command(0x12, 0x07);

    ///////////////////////////
    // GPIO4:RST
    // GPIO3:x
    // GPIO2:SPK_EN
    // GPIO1:SYS_LED
    // GPIO0:BUS_PW_EN
    ///////////////////////////
    // GPIO[4:3] mode
    //      GPIO3: x
    //      GPIO4: output(open drain)
    write_command(0x95, 0x84);
    // GPIO2 mode
    //      GPIO2: output(open drain)
    write_command(0x93, 0x00);
    // GPIO0 volt
    //      3300 ==> (3300 - 1800)/100 ==> 15
    //      3.3V: [7:4]=1111
    write_command(0x91, 0xf0);
    // ADC enable
    write_command(0x82, 0xfe);

    // system LED
    power_led(0);

    // enable
    //      EXT     [7]=1
    //              [6:3]=0000
    //      LDO2    [2]=1
    //      DCDC3   [1]=1
    //      DCDC1   [0]=1
    write_command(0x12, 0x87);
    // GPIO0 mode
    //              [7:3]=00000
    //      LDO     [2:0]=010
    write_command(0x90, 0x02);

    return ESP_OK;
}

static esp_err_t master_write_slave(uint8_t address, uint8_t* data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    I2C_LOG_ANY_ERROR(i2c_master_start(cmd));
    I2C_LOG_ANY_ERROR(i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true));
    I2C_LOG_ANY_ERROR(i2c_master_write(cmd, data_wr, size, true));
    I2C_LOG_ANY_ERROR(i2c_master_stop(cmd));

    const esp_err_t ret = i2c_master_cmd_begin(I2C_BATTERY_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    I2C_LOG_ANY_ERROR(ret);

    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t master_read_slave(uint8_t address, uint8_t register_address, uint8_t* data_rd, size_t size)
{
    if (size == 0) {
        return ESP_OK;
    }

    I2C_CHECK_RET(master_write_slave(address, &register_address, 1));

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    I2C_LOG_ANY_ERROR(i2c_master_start(cmd));
    I2C_LOG_ANY_ERROR(i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, ACK_CHECK_EN));
    if (size > 1) {
        I2C_LOG_ANY_ERROR(i2c_master_read(cmd, data_rd, size - 1, ACK_VAL));
    }

    I2C_LOG_ANY_ERROR(i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL));
    I2C_LOG_ANY_ERROR(i2c_master_stop(cmd));

    const esp_err_t ret = i2c_master_cmd_begin(I2C_BATTERY_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    I2C_LOG_ANY_ERROR(ret);

    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t write_command(uint8_t reg, uint8_t val)
{
    uint8_t arr[] = { reg, val };
    I2C_CHECK_RET(master_write_slave(0x34, arr, 2));
    vTaskDelay(20 / portTICK_PERIOD_MS);
    return ESP_OK;
}

// esp_err_t power_screen_on() { return write_command(0x90, 0x02); }
// esp_err_t power_screen_off() { return write_command(0x90, 0x01); }
// esp_err_t power_set_camera_voltage() { return write_command(0x28, 0xf0); }
// esp_err_t power_enable_dc_dc1() { return write_command(0x12, 0x4d); }
// esp_err_t power_enable_dc_dc2() { return write_command(0x10, 0xff); }
// esp_err_t power_enable_adcs() { return write_command(0x82, 0xff); }
// esp_err_t power_enable_charging() { return write_command(0x33, 0xc0); }
// esp_err_t power_enable_coulomb_counter() { return write_command(0xb8, 0x80); }
// esp_err_t power_setup_pek() { return write_command(0x36, 0x5c); }
// esp_err_t power_set_v_off() { return write_command(0x31, 0x04); }

esp_err_t power_screen_on() { return ESP_OK; }
esp_err_t power_screen_off() { return ESP_OK; }
esp_err_t power_set_camera_voltage() { return ESP_OK; }
esp_err_t power_enable_dc_dc1() { return ESP_OK; }
esp_err_t power_enable_dc_dc2() { return ESP_OK; }
esp_err_t power_enable_adcs() { return ESP_OK; }
esp_err_t power_enable_charging() { return ESP_OK; }
esp_err_t power_enable_coulomb_counter() { return ESP_OK; }
esp_err_t power_setup_pek() { return ESP_OK; }
esp_err_t power_set_v_off() { return ESP_OK; }

// 0x95: GPIO[4:3]機能設定
// 0x96: GPIO[4:3]値設定
esp_err_t power_open_drain_gpio() { return write_command(0x95, 0x05); }

// GPIO[4:3]=01 ... M5CORE2 DISPリセット
esp_err_t power_lcd_reset_on() { return write_command(0x96, 0x01); }
// GPIO[4:3]=11 ... M5CORE2 DISPリセット解除
esp_err_t power_lcd_reset_off() { return write_command(0x96, 0x03); }

esp_err_t power_shutdown() { return write_command(0x32, 0x80); }

esp_err_t power_led(uint8_t onoff) {
    // uint8_t buf;
    // I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x94, &buf, 1));
    // // GPIO1
    // if (onoff) {
    //     buf &= ~0x02;
    // } else {
    //     buf |= 0x02;
    // }
    // return write_command(0x94, buf);
    if (onoff) {
        led = true;
        return write_command(0x92, 0x00);  //GPIO1 ... OUTPUT
    } else {
        led = false;
        return write_command(0x92, 0x01);  //GPIO1 ... INPUT
    }
}

void power_led_blink(void)
{
    if (led) {
        power_led(0);
    } else {
        power_led(1);
    }
}

esp_err_t power_speaker(uint8_t onoff) {
    uint8_t buf;
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x94, &buf, 1));
    // GPIO2
    if (onoff) {
        buf |= 0x04;
    } else {
        buf &= ~0x04;
    }
    return write_command(0x94, buf);
}

uint16_t power_get_vbat()
{
    uint16_t vbat = 0;
    uint8_t buf1, buf2;
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x78, &buf1, 1));
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x79, &buf2, 1));
    vbat = ((buf1 << 4) + buf2) * 1.1;
    return vbat;
}

uint8_t power_get_battery_status()
{
    const uint16_t vbat = power_get_vbat();
    if (vbat > 4000) {
        return 5;
    } else if (vbat > 3850) {
        return 4;
    } else if (vbat > 3700) {
        return 3;
    } else if (vbat > 3550) {
        return 2;
    } else if (vbat > 3400) {
        return 1;
    }
    return 0;
}

bool power_get_battery_charging()
{
    bool charging = false;
    uint8_t buf;
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x01, &buf, 1));
    charging = (buf & 0b01000000) >> 6;
    return charging;
}

uint16_t power_get_ibat_charge()
{
    uint16_t ibat = 0;
    uint8_t buf1, buf2;
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x7A, &buf1, 1));
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x7B, &buf2, 1));
    ibat = (buf1 << 5) + buf2;
    return ibat;
}

uint16_t power_get_ibat_discharge()
{
    uint16_t ibat = 0;
    uint8_t buf1, buf2;
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x7C, &buf1, 1));
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x7D, &buf2, 1));
    ibat = (buf1 << 5) + buf2;
    return ibat;
}

uint16_t power_get_vusb()
{
    uint16_t vusb = 0;
    uint8_t buf1, buf2;
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x5a, &buf1, 1));
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x5b, &buf2, 1));
    vusb = ((buf1 << 4) + buf2) * 1.7;
    return vusb;
}

uint16_t power_get_iusb()
{
    uint16_t iusb = 0;
    uint8_t buf1, buf2;
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x5c, &buf1, 1));
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x5d, &buf2, 1));
    iusb = ((buf1 << 4) + buf2) * 0.375;
    return iusb;
}

uint16_t power_get_temp()
{
    uint16_t temp = 0;
    uint8_t buf1, buf2;
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x5e, &buf1, 1));
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x5f, &buf2, 1));
    temp = ((buf1 << 4) + buf2) * 0.1 - 144.7;
    return temp;
}

bool usb_connected()
{
    bool is_usb_connected = false;
    uint8_t buf;
    I2C_LOG_ANY_ERROR(master_read_slave(0x34, 0x00, &buf, 1));
    is_usb_connected = buf & 0b00100000;
    return is_usb_connected;
}
