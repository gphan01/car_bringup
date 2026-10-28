#include "../steering.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

typedef struct
{
    uint16_t last_us;
    int calls;
    bool fail;
} fake_t;

static steering_cal_t good_cal(void)
{
    return (steering_cal_t){.left_us = 1900, .center_us = 1500, .right_us = 1100};
}

static steering_cal_t bad_cal(void)
{
    return (steering_cal_t){.left_us = 1900, .center_us = 2002, .right_us = 1101};
}

static steering_status_t fake_write(void *ctx, uint16_t pulse_ms)
{
    fake_t *f = (fake_t *)ctx;

    if (f->fail == 1)
        return STEERING_ERR_IO;
    else
    {
        f->last_us = pulse_ms;
        f->calls++;
    }

    return STEERING_OK;
}

static void make_dev(steering_dev_t *dev, fake_t *f)
{
    *dev = (steering_dev_t){.write = fake_write, .ctx = f};
}

static void test_steering_init_rejects(void)
{
    fake_t fake = {0};
    steering_dev_t dev;
    steering_cal_t cal = good_cal();
    steering_status_t st = steering_init(NULL, &cal);
    assert(st == STEERING_ERR_PARAM);

    /* Valid dev, NULL cal*/
    make_dev(&dev, &fake);
    st = steering_init(&dev, NULL);
    assert(st == STEERING_ERR_PARAM);
    assert(fake.fail == 0);

    /* NULL write fn: valid cal, should retunr ERR_PARAM*/
    steering_dev_t d = {.write = NULL};
    cal = good_cal();
    st = steering_init(&d, &cal);
    assert(st == STEERING_ERR_PARAM);

    /* Out-of-range vlaue: valid dev, one bad field*/
    make_dev(&dev, &fake);
    cal = good_cal();
    cal.left_us = 900;
    st = steering_init(&dev, &cal);
    assert(st == STEERING_ERR_PARAM);
    assert(fake.calls == 0); /*Reject before anything was written*/
}

static void test_steering_init_reject_bad_cal(void)
{
    fake_t f = {0};
    steering_dev_t dev;
    steering_cal_t cal = bad_cal();

    make_dev(&dev, &f);

    steering_status_t st = steering_init(&dev, &cal);
    assert(st == STEERING_ERR_PARAM);
    assert(f.calls == 0);
}

static void test_steering_init_reject_out_range(void)
{
    fake_t f = {0};
    steering_dev_t dev;
    steering_cal_t cal = {.left_us = 900, .center_us = 1500, .right_us = 1100};

    make_dev(&dev, &f);
    steering_status_t st = steering_init(&dev, &cal);
    assert(st == STEERING_ERR_PARAM);
    assert(f.calls == 0);
}

static void test_steering_init_accepts_left_low(void)
{
    fake_t f = {0};
    steering_dev_t dev;
    steering_cal_t cal = {.left_us = 1100, .center_us = 1500, .right_us = 1900};

    make_dev(&dev, &f);
    steering_status_t st = steering_init(&dev, &cal);
    assert(st == STEERING_OK);
    assert(f.calls == 1);
}

static void test_steering_init_accepts_left_high(void)
{
    fake_t f = {0};
    steering_dev_t dev;
    steering_cal_t cal = {.left_us = 1900, .center_us = 1500, .right_us = 1100};

    make_dev(&dev, &f);
    steering_status_t st = steering_init(&dev, &cal);
    assert(f.last_us == cal.center_us);
    assert(st == STEERING_OK);
    assert(f.calls == 1);
}

static void test_set_pulse_before_init(void)
{
    fake_t f = {0};
    steering_dev_t dev;

    make_dev(&dev, &f);

    steering_status_t st = steering_set_pulse_us(&dev, 2000);
    assert(st == STEERING_ERR_NOT_INIT);
    assert(f.calls == 0);
}

static void check_clamps(steering_cal_t cal)
{
    fake_t f = {0};
    steering_dev_t dev;
    make_dev(&dev, &f);

    steering_status_t st = steering_init(&dev, &cal);
    assert(st == STEERING_OK);
    f = (fake_t){0};

    /* High clamp*/
    st = steering_set_pulse_us(&dev, 2100);
    assert(st == STEERING_OK);
    assert(f.last_us == 1900);
    assert(dev.clamp_count == 1);
    assert(f.calls == 1);

    /* Low clamp*/
    st = steering_set_pulse_us(&dev, 600);
    assert(st == STEERING_OK);
    assert(f.last_us == 1100);
    assert(dev.clamp_count == 2);
    assert(f.calls == 2);

    /* In-range*/
    st = steering_set_pulse_us(&dev, 1600);
    assert(st == STEERING_OK);
    assert(f.last_us == 1600);
    assert(dev.clamp_count == 2);
    assert(f.calls == 3);
}

static void test_set_pulse_setter_clamps_both_orders(void)
{
    /* Check both orders*/
    check_clamps((steering_cal_t){.left_us = 1900, .center_us = 1500, .right_us = 1100});
    check_clamps((steering_cal_t){.left_us = 1100, .center_us = 1500, .right_us = 1900});
}

static void check_angle(float angle, uint16_t expect_us, uint32_t expect_clamps)
{
    fake_t f = {0};
    steering_dev_t dev;
    steering_cal_t cal = {.left_us = 1000,
                          .center_us = 1500,
                          .right_us = 2000,
                          .max_left_rad = 0.40f,
                          .max_right_rad = 0.40f};
    make_dev(&dev, &f);
    steering_status_t st = steering_init(&dev, &cal);
    assert(st == STEERING_OK);
    f = (fake_t){0};

    st = steering_set_angle(&dev, angle);

    assert(st == STEERING_OK);
    assert(f.last_us == expect_us);
    assert(dev.clamp_count == expect_clamps);
}

static void test_set_angle(void)
{
    check_angle(0.00f, 1500, 0);
    check_angle(0.20f, 1250, 0);
    check_angle(-0.20f, 1750, 0);
    check_angle(0.40f, 1000, 0); /* exactly max: not a clamp */
    check_angle(0.60f, 1000, 1); /* beyond max: saturates */
    check_angle(-0.60f, 2000, 1);
}

static void test_set_angle_rejects_nan(void)
{
    fake_t f = {0};
    steering_dev_t dev;
    steering_cal_t cal = {.left_us = 1000,
                          .center_us = 1500,
                          .right_us = 2000,
                          .max_left_rad = 0.40f,
                          .max_right_rad = 0.40f};
    make_dev(&dev, &f);
    steering_status_t st = steering_init(&dev, &cal);
    assert(st == STEERING_OK);
    f = (fake_t){0};

    st = steering_set_angle(&dev, NAN);

    assert(st == STEERING_ERR_PARAM);
    assert(f.calls == 0);
}

int main(void)
{
    test_steering_init_rejects();
    test_steering_init_reject_bad_cal();
    test_steering_init_reject_out_range();
    test_steering_init_accepts_left_low();
    test_steering_init_accepts_left_high();
    test_set_pulse_before_init();
    test_set_pulse_setter_clamps_both_orders();
    test_set_angle();
    test_set_angle_rejects_nan();

    printf("All tests passed.\n");
    return 0;
}
