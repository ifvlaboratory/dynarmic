/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <catch.hpp>

#include <dynarmic/A32/a32.h>
#include <dynarmic/exclusive_monitor.h>

#include "common/common_types.h"
#include "testenv.h"
#include "arm_dynarmic_cp15.h"

static std::shared_ptr<DynarmicCP15> cp15 = nullptr;
static Dynarmic::ExclusiveMonitor *monitor = nullptr;

static Dynarmic::A32::UserConfig GetUserConfig(ThumbTestEnv* testenv) {
    Dynarmic::A32::UserConfig user_config;
    user_config.callbacks = testenv;
    user_config.processor_id = 0;
    if(monitor) {
        user_config.global_monitor = monitor;
    }
    if(cp15) {
        user_config.coprocessors[15] = cp15;
    }
    return user_config;
}

TEST_CASE("thumb2: MLA", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xfb03, 0x0302, // mla r3, r3, r2, r0
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[2] = 2;
    jit.Regs()[3] = 3;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[3] == 7);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: LDREX", "[thumb2]") {
    monitor = new Dynarmic::ExclusiveMonitor(1);

    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xe854, 0x3f00, // ldrex r3, [r4]
            0xe844, 0x9100, // strex r1, sb, [r4]
            0xe7fe, // b #0
    };

    jit.Regs()[1] = 7;
    jit.Regs()[3] = 3;
    jit.Regs()[4] = 0x78;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.Regs()[1] == 0);
    REQUIRE(jit.Regs()[3] == 0x7b7a7978);
    REQUIRE(jit.Regs()[15] == 8);
    REQUIRE(jit.Cpsr() == 0x00000030);

    delete monitor;
    monitor = nullptr;
}

TEST_CASE("thumb2: VMOV (2xcore to f64)", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xec45, 0x4b31, // vmov d17, r4, r5
            0xeff4, 0x00b1, // vshr.s64 d16, d17, #0xc
            0xec51, 0x0b30, // vmov r0, r1, d16
            0xe7fe, // b #0
    };

    jit.Regs()[4] = 0x12345678;
    jit.Regs()[5] = 0x78563412;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 3;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x41212345);
    REQUIRE(jit.Regs()[1] == 0x00078563);
    REQUIRE(jit.ExtRegs()[32] == 0x41212345);
    REQUIRE(jit.ExtRegs()[33] == 0x00078563);
    REQUIRE(jit.ExtRegs()[34] == 0x12345678);
    REQUIRE(jit.ExtRegs()[35] == 0x78563412);
    REQUIRE(jit.Regs()[15] == 12);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: STRD (imm)", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xe9e2, 0x0102, // strd r0, r1, [r2, #0x8]!
            0xe9d2, 0x3400, // ldrd r3, r4, [r2]
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 0x12345678;
    jit.Regs()[1] = 0x17654320;
    jit.Regs()[2] = 0x78;
    jit.Regs()[3] = 3;
    jit.Regs()[4] = 4;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.Regs()[2] == 0x80);
    REQUIRE(jit.Regs()[3] == 0x12345678);
    REQUIRE(jit.Regs()[4] == 0x17654320);
    REQUIRE(jit.Regs()[15] == 8);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: LDRD (imm)", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xe9f2, 0x0102, // ldrd r0, r1, [r2, #0x8]!
            0xe8f5, 0x3402, // ldrd r3, r4, [r5], #0x8
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[2] = 0x78;
    jit.Regs()[3] = 3;
    jit.Regs()[4] = 4;
    jit.Regs()[5] = 0x78;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x83828180);
    REQUIRE(jit.Regs()[1] == 0x87868584);
    REQUIRE(jit.Regs()[2] == 0x80);
    REQUIRE(jit.Regs()[3] == 0x7b7a7978);
    REQUIRE(jit.Regs()[4] == 0x7f7e7d7c);
    REQUIRE(jit.Regs()[5] == 0x80);
    REQUIRE(jit.Regs()[15] == 8);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: LDREXH", "[thumb2]") {
    monitor = new Dynarmic::ExclusiveMonitor(1);

    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xe8d2, 0x1f5f, // ldrexh r1, [r2]
            0xe8c2, 0x0f53, // strexh r3, r0, [r2]
            0x6812, // ldr r2, [r2]
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[2] = 0x78;
    jit.Regs()[3] = 3;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 4;
    jit.Run();

    REQUIRE(jit.Regs()[1] == 0x7978);
    REQUIRE(jit.Regs()[2] == 0x7b7a0001);
    REQUIRE(jit.Regs()[3] == 0);
    REQUIRE(jit.Regs()[15] == 10);
    REQUIRE(jit.Cpsr() == 0x00000030);

    delete monitor;
    monitor = nullptr;
}

TEST_CASE("thumb2: PUSH", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xe92d, 0x0018, // push.w {r3,r4}
            0xbc03, // pop {r0,r1}
            0xe92d, 0x0006, // push.w {r1,r2}
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[3] = 3;
    jit.Regs()[4] = 4;
    jit.Regs()[13] = 24; // SP = 24
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 3;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 3);
    REQUIRE(jit.Regs()[1] == 4);
    REQUIRE(jit.Regs()[13] == 16);
    REQUIRE(jit.Regs()[15] == 10);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: MRC", "[thumb2]") {
    cp15 = std::make_shared<DynarmicCP15>();

    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xee1d, 0x0f70, // mrc p15, 0, r0, c13, c0, 3
            0xE7FE, // b +#0
    };

    cp15->uro = 0x12345678;
    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x12345678);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);

    cp15 = nullptr;
}

TEST_CASE("thumb: UXTH", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xb281, // uxth r1, r0
            0xE7FE, // b +#0
    };

    jit.Regs()[0] = 0x12345678;
    jit.Regs()[1] = 2;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x12345678);
    REQUIRE(jit.Regs()[1] == 0x5678);
    REQUIRE(jit.Regs()[15] == 2);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: UXTH", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xfa1f, 0xf180, // uxth.w r1, r0
            0xfa1f, 0xf290, // uxth.w r2, r0, ror #8
            0xE7FE, // b +#0
    };

    jit.Regs()[0] = 0x12345678;
    jit.Regs()[1] = 2;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x12345678);
    REQUIRE(jit.Regs()[1] == 0x5678);
    REQUIRE(jit.Regs()[2] == 0x3456);
    REQUIRE(jit.Regs()[15] == 8);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb: lsls r0, r1, #2", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0x0088, // lsls r0, r1, #2
        0xE7FE, // b +#0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 8);
    REQUIRE(jit.Regs()[1] == 2);
    REQUIRE(jit.Regs()[15] == 2);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb: lsls r0, r1, #31", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0x07C8, // lsls r0, r1, #31
        0xE7FE, // b +#0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 0xFFFFFFFF;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x80000000);
    REQUIRE(jit.Regs()[1] == 0xffffffff);
    REQUIRE(jit.Regs()[15] == 2);
    REQUIRE(jit.Cpsr() == 0xA0000030); // N, C flags, Thumb, User-mode
}

TEST_CASE("thumb: revsh r4, r3", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xBADC, // revsh r4, r3
        0xE7FE, // b +#0
    };

    jit.Regs()[3] = 0x12345678;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[3] == 0x12345678);
    REQUIRE(jit.Regs()[4] == 0x00007856);
    REQUIRE(jit.Regs()[15] == 2);
    REQUIRE(jit.Cpsr() == 0x00000030); // Thumb, User-mode
}

TEST_CASE("thumb: ldr r3, [r3, #28]", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0x69DB, // ldr r3, [r3, #28]
        0xE7FE, // b +#0
    };

    jit.Regs()[3] = 0x12345678;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[3] == 0x97969594); // Memory location 0x12345694
    REQUIRE(jit.Regs()[15] == 2);
    REQUIRE(jit.Cpsr() == 0x00000030); // Thumb, User-mode
}

TEST_CASE("thumb: blx +#67712", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xF010, 0xEC3E, // blx +#67712
        0xE7FE          // b +#0
    };

    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[14] == (0x4U | 1U));
    REQUIRE(jit.Regs()[15] == 0x10880);
    REQUIRE(jit.Cpsr() == 0x00000010); // User-mode
}

TEST_CASE("thumb: bl +#234584", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xF039, 0xFA2A, // bl +#234584
        0xE7FE          // b +#0
    };

    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[14] == (0x4U | 1U));
    REQUIRE(jit.Regs()[15] == 0x39458);
    REQUIRE(jit.Cpsr() == 0x00000030); // Thumb, User-mode
}

TEST_CASE("thumb: bl -#42", "[thumb]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xF7FF, 0xFFE9, // bl -#42
        0xE7FE          // b +#0
    };

    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[14] == (0x4U | 1U));
    REQUIRE(jit.Regs()[15] == 0xFFFFFFD6);
    REQUIRE(jit.Cpsr() == 0x00000030); // Thumb, User-mode
}

TEST_CASE("thumb2: CLZ", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xfab3, 0xf083, // clz r0, r3
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[3] = 3;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 30);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: UDIV", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xfbb3, 0xfcf4, // udiv ip, r3, r4
            0xe7fe, // b #0
    };

    jit.Regs()[3] = 300;
    jit.Regs()[4] = 4;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[12] == 75);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: MUL", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xfb00, 0xf201, // mul r2, r0, r1
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 10;
    jit.Regs()[1] = 20;
    jit.Regs()[2] = 30;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[2] == 200);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: MLS", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xfb01, 0x3012, // mls r0, r1, r2, r3
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[2] = 3;
    jit.Regs()[3] = 40;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 34);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: LSR (reg)", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xfa21, 0xf002, // lsr.w r0, r1, r2
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 0xffff;
    jit.Regs()[2] = 3;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x1fff);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: LSL (reg)", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xfa01, 0xf002, // lsl.w r0, r1, r2
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 0xffff;
    jit.Regs()[2] = 3;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == (0xffffU << 3U));
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: TBH", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xe8df, 0xf010, // tbh [pc, r0, lsl #1]
            0x021a, 0x009e
    };

    jit.Regs()[0] = 1;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[15] == 0x140);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: RBIT", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xfa91, 0xf0a1, // rbit r0, r1
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 0x12345678;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 0x1e6a2c48);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: UBFX", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xf3c1, 0x1007, // rbit r0, r1
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 0x12345678;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 103);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: VBIC, VMOV, VMVN, VORR (immediate)", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xefc0, 0x0010, // vmov.i32 d16, #0
            0xe7fe, // b #0
    };

    jit.ExtRegs()[32] = 32;
    jit.ExtRegs()[33] = 33;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE(jit.ExtRegs()[32] == 0);
    REQUIRE(jit.ExtRegs()[33] == 0);
    REQUIRE(jit.Regs()[15] == 4);
    REQUIRE(jit.Cpsr() == 0x00000030);
}

TEST_CASE("thumb2: VSTR", "[thumb2]") {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
            0xedcd, 0x0b00, // vstr d16, [sp]
            0xe9dd, 0x0100, // ldrd r0, r1, [sp]
            0xe7fe, // b #0
    };

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.ExtRegs()[32] = 32;
    jit.ExtRegs()[33] = 33;
    jit.Regs()[13] = 0x10; // SP
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 2;
    jit.Run();

    REQUIRE(jit.Regs()[0] == 32);
    REQUIRE(jit.Regs()[1] == 33);
    REQUIRE(jit.Regs()[15] == 8);
    REQUIRE(jit.Cpsr() == 0x00000030);
}
