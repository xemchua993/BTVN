#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define BMP180_ADDR 0x77
#define DEVICE_NAME "bmp180"
#define CLASS_NAME  "bmp_class"

static struct i2c_client *bmp180_client;
static struct class *bmp_class;
static struct cdev bmp_cdev;
static dev_t dev_num;

struct bmp180_calib_param {
    s16 ac1, ac2, ac3, b1, b2, mb, mc, md;
    u16 ac4, ac5, ac6;
};

static struct bmp180_calib_param calib;

static int bmp180_read_word(u8 reg, s16 *val)
{
    s32 msb = i2c_smbus_read_byte_data(bmp180_client, reg);
    s32 lsb = i2c_smbus_read_byte_data(bmp180_client, reg + 1);
    if (msb < 0 || lsb < 0) return -EIO;
    *val = (msb << 8) | lsb;
    return 0;
}

static int bmp180_read_calibration(void)
{
    int ret = 0;
    ret |= bmp180_read_word(0xAA, &calib.ac1);
    ret |= bmp180_read_word(0xAC, &calib.ac2);
    ret |= bmp180_read_word(0xAE, &calib.ac3);
    ret |= bmp180_read_word(0xB0, (s16 *)&calib.ac4);
    ret |= bmp180_read_word(0xB2, (s16 *)&calib.ac5);
    ret |= bmp180_read_word(0xB4, (s16 *)&calib.ac6);
    ret |= bmp180_read_word(0xB6, &calib.b1);
    ret |= bmp180_read_word(0xB8, &calib.b2);
    ret |= bmp180_read_word(0xBA, &calib.mb);
    ret |= bmp180_read_word(0xBC, &calib.mc);
    ret |= bmp180_read_word(0xBE, &calib.md);
    return ret;
}

static int bmp180_get_temperature(int *t)
{
    s32 ut, x1, x2, b5;
    s16 val;

    if (i2c_smbus_write_byte_data(bmp180_client, 0xF4, 0x2E) < 0)
        return -EIO;
    msleep(5);

    if (bmp180_read_word(0xF6, &val) < 0)
        return -EIO;

    ut = val;
    x1 = ((ut - calib.ac6) * calib.ac5) >> 15;
    x2 = ((s32)calib.mc << 11) / (x1 + calib.md);
    b5 = x1 + x2;
    *t = (b5 + 8) >> 4;
    return 0;
}

static int bmp180_get_pressure(int oss, int *p)
{
    s32 ut, up, x1, x2, x3, b3, b5, b6, p_raw;
    u32 b4, b7;
    u8 buf[3];

    // Temperature first (needed for pressure calc)
    if (bmp180_get_temperature(&ut) < 0)
        return -EIO;

    // Trigger pressure measurement
    if (i2c_smbus_write_byte_data(bmp180_client, 0xF4, 0x34 + (oss << 6)) < 0)
        return -EIO;

    msleep(2 + (3 << oss));  // Wait time varies by mode

    if (i2c_smbus_read_i2c_block_data(bmp180_client, 0xF6, 3, buf) != 3)
        return -EIO;

    up = ((buf[0] << 16) | (buf[1] << 8) | buf[2]) >> (8 - oss);

    // Calculate pressure
    x1 = ((ut - calib.ac6) * calib.ac5) >> 15;
    x2 = ((s32)calib.mc << 11) / (x1 + calib.md);
    b5 = x1 + x2;
    b6 = b5 - 4000;
    x1 = (calib.b2 * (b6 * b6 >> 12)) >> 11;
    x2 = (calib.ac2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = ((((s32)calib.ac1 * 4 + x3) << oss) + 2) >> 2;
    x1 = (calib.ac3 * b6) >> 13;
    x2 = (calib.b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (calib.ac4 * (u32)(x3 + 32768)) >> 15;
    b7 = ((u32)up - b3) * (50000 >> oss);
    if (b7 < 0x80000000)
        p_raw = (b7 << 1) / b4;
    else
        p_raw = (b7 / b4) << 1;

    x1 = (p_raw >> 8) * (p_raw >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p_raw) >> 16;
    *p = p_raw + ((x1 + x2 + 3791) >> 4);
    return 0;
}

static ssize_t bmp180_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    int temp, pressure;
    char msg[128];
    int ret;

    if (*off > 0) {
        return 0;  // No more data
    }

    ret = bmp180_get_temperature(&temp);
    if (ret < 0)
        return ret;

    ret = bmp180_get_pressure(3, &pressure);  // ultra high res
    if (ret < 0)
        return ret;

    snprintf(msg, sizeof(msg), "Temp: %d.%d C\nPressure: %d Pa\n",
             temp / 10, abs(temp % 10), pressure);

    return simple_read_from_buffer(buf, len, off, msg, strlen(msg));
}

static int bmp180_open(struct inode *inode, struct file *file) { return 0; }
static int bmp180_release(struct inode *inode, struct file *file) { return 0; }

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = bmp180_open,
    .read = bmp180_read,
    .release = bmp180_release,
};

static int bmp180_probe(struct i2c_client *client)
{
    int ret;

    bmp180_client = client;

    // Đọc dữ liệu hiệu chuẩn
    ret = bmp180_read_calibration();
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read calibration data\n");
        return ret;
    }

    // Đăng ký vùng thiết bị và tạo cdev
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME); // Đăng ký thiết bị
    if (ret < 0) {
        dev_err(&client->dev, "Failed to allocate chrdev region\n");
        return ret;
    }

    cdev_init(&bmp_cdev, &fops);
    bmp_cdev.owner = THIS_MODULE;

    ret = cdev_add(&bmp_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1); // Nếu không thêm được cdev, hủy đăng ký thiết bị
        return ret;
    }

    // Tạo lớp thiết bị
    bmp_class = class_create(CLASS_NAME);
    if (IS_ERR(bmp_class)) {
        cdev_del(&bmp_cdev); // Nếu tạo lớp thiết bị thất bại, xóa cdev
        unregister_chrdev_region(dev_num, 1); // Hủy đăng ký thiết bị
        return PTR_ERR(bmp_class);
    }

    // Tạo node thiết bị
    device_create(bmp_class, NULL, dev_num, NULL, DEVICE_NAME);

    dev_info(&client->dev, "BMP180 driver loaded, device created at /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void bmp180_remove(struct i2c_client *client)
{
    // Xóa node thiết bị và hủy lớp thiết bị
    device_destroy(bmp_class, dev_num);
    class_destroy(bmp_class);

    // Xóa cdev và hủy đăng ký thiết bị
    cdev_del(&bmp_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("bmp180: removed\n");
}

static const struct of_device_id bmp180_of_match[] = {
    { .compatible = "bosch,bmp180", },
    {},
};
MODULE_DEVICE_TABLE(of, bmp180_of_match);

static struct i2c_driver bmp180_driver = {
    .driver = {
        .name = "bmp180",
        .of_match_table = bmp180_of_match,
    },
    .probe = bmp180_probe,
    .remove = bmp180_remove,
};

module_i2c_driver(bmp180_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("BMP180 Sensor Driver");
MODULE_VERSION("1.0");
