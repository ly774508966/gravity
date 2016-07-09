/*
 *  Duktape current coerces writeUInt8() etc arguments like TypedArray
 *  (silently) rather than throwing a TypeError for out-of-range values.
 *
 *  Node.js Buffer seems to do a range check, but -will- silently coerce
 *  fractional values within the range.
 */

/*@include util-nodejs-buffer.js@*/

/*---
{
    "custom": true
}
---*/

/*===
-Infinity 00000000000000000000000000000000
-1000000000000000 00000000000000000000000000000000
-4294967295 00000001000000000000000000000000
-32769 000000ff000000000000000000000000
-32768 00000000000000000000000000000000
-129 0000007f000000000000000000000000
-128 00000080000000000000000000000000
-10.9 000000f6000000000000000000000000
-10.5 000000f6000000000000000000000000
-10.1 000000f6000000000000000000000000
-10 000000f6000000000000000000000000
-9.9 000000f7000000000000000000000000
-9.5 000000f7000000000000000000000000
-9.1 000000f7000000000000000000000000
-9 000000f7000000000000000000000000
-1 000000ff000000000000000000000000
0 00000000000000000000000000000000
0 00000000000000000000000000000000
1 00000001000000000000000000000000
9 00000009000000000000000000000000
9.1 00000009000000000000000000000000
9.5 00000009000000000000000000000000
9.9 00000009000000000000000000000000
10 0000000a000000000000000000000000
10.1 0000000a000000000000000000000000
10.5 0000000a000000000000000000000000
10.9 0000000a000000000000000000000000
127 0000007f000000000000000000000000
128 00000080000000000000000000000000
32767 000000ff000000000000000000000000
32768 00000000000000000000000000000000
4294967295 000000ff000000000000000000000000
1000000000000000 00000000000000000000000000000000
Infinity 00000000000000000000000000000000
NaN 00000000000000000000000000000000
-Infinity 00000000000000000000000000000000
-1000000000000000 00000080000000000000000000000000
-4294967295 00000000010000000000000000000000
-32769 0000007fff0000000000000000000000
-32768 00000080000000000000000000000000
-129 000000ff7f0000000000000000000000
-128 000000ff800000000000000000000000
-10.9 000000fff60000000000000000000000
-10.5 000000fff60000000000000000000000
-10.1 000000fff60000000000000000000000
-10 000000fff60000000000000000000000
-9.9 000000fff70000000000000000000000
-9.5 000000fff70000000000000000000000
-9.1 000000fff70000000000000000000000
-9 000000fff70000000000000000000000
-1 000000ffff0000000000000000000000
0 00000000000000000000000000000000
0 00000000000000000000000000000000
1 00000000010000000000000000000000
9 00000000090000000000000000000000
9.1 00000000090000000000000000000000
9.5 00000000090000000000000000000000
9.9 00000000090000000000000000000000
10 000000000a0000000000000000000000
10.1 000000000a0000000000000000000000
10.5 000000000a0000000000000000000000
10.9 000000000a0000000000000000000000
127 000000007f0000000000000000000000
128 00000000800000000000000000000000
32767 0000007fff0000000000000000000000
32768 00000080000000000000000000000000
4294967295 000000ffff0000000000000000000000
1000000000000000 00000080000000000000000000000000
Infinity 00000000000000000000000000000000
NaN 00000000000000000000000000000000
===*/

function writeCoercionTest() {
    // Just an example for Uint8 and Int16.

    var b = new Buffer(16);

    function u8(v) {
        b.fill(0x00);
        try {
            b.writeUInt8(v, 3);
        } catch (e) {
            print(v, e.name);
        }
        print(v, printableNodejsBuffer(b));
    }

    function i16(v) {
        b.fill(0x00);
        try {
            b.writeUInt16BE(v, 3);
        } catch (e) {
            print(v, e.name);
        }
        print(v, printableNodejsBuffer(b));
    }

    var values = [
        -1/0, -1e15, -0xffffffff, -0x8001, -0x8000,
        -0x81, -0x80, -10.9, -10.5, -10.1, -10, -9.9, -9.5, -9.1, -9,
        -1, -0, 0, 1, 9, 9.1, 9.5, 9.9, 10, 10.1, 10.5, 10.9,
        0x7f, 0x80, 0x7fff, 0x8000, 0xffffffff, 1e15, 1/0, 0/0
    ];

    values.forEach(u8);
    values.forEach(i16);
}

try {
    writeCoercionTest();
} catch (e) {
    print(e.stack || e);
}
