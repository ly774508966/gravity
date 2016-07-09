// return value does not matter
function retUndef(val, key, obj) {
    print(typeof this, this, typeof val, val, typeof key, key, typeof obj, obj);
}

function test(this_value, args) {
    var t;

    try {
        t = Array.prototype.forEach.apply(this_value, args);
        print(typeof t, t);
    } catch (e) {
        print(e.name);
    }
}

/*===
basic
undefined undefined
object [object global] number 1 number 0 object 1
undefined undefined
object [object global] number 1 number 0 object 1,2
object [object global] number 2 number 1 object 1,2
undefined undefined
object [object global] number 1 number 0 object 1,2,3,4,5
object [object global] number 2 number 1 object 1,2,3,4,5
object [object global] number 3 number 2 object 1,2,3,4,5
object [object global] number 4 number 3 object 1,2,3,4,5
object [object global] number 5 number 4 object 1,2,3,4,5
undefined undefined
object [object global] number 1 number 0 object 1,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,2,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,3
object [object global] number 2 number 50 object 1,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,2,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,3
object [object global] number 3 number 100 object 1,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,2,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,3
undefined undefined
object [object global] string foo number 0 object [object Object]
object [object global] string bar number 5 object [object Object]
object [object global] string quux number 20 object [object Object]
undefined undefined
callback 0
callback 1
callback 2
callback 3
callback 4
callback 5
callback 6
callback 7
callback 8
callback 9
undefined undefined
callback 1
CallbackError
nonstrict object
nonstrict object
nonstrict object
undefined undefined
nonstrict object
nonstrict object
nonstrict object
undefined undefined
nonstrict object
nonstrict object
nonstrict object
undefined undefined
strict undefined undefined
strict undefined undefined
strict undefined undefined
undefined undefined
strict object null
strict object null
strict object null
undefined undefined
strict string foo
strict string foo
strict string foo
undefined undefined
===*/

print('basic');

function basicTest() {
    var obj;
    var count;

    // simple cases

    test([], [ retUndef ]);
    test([1], [ retUndef ]);
    test([1,2], [ retUndef ]);

    // dense

    test([1,2,3,4,5], [ retUndef ]);

    // sparse

    obj = [1];
    obj[100] = 3;
    obj[50] = 2;
    test(obj, [ retUndef ]);

    // non-array

    obj = { '0': 'foo', '5': 'bar', '20': 'quux', '100': 'baz', length: 35 };
    test(obj, [ retUndef ]);

    // return value doesn't matter, return value is not coerced

    count = 3;
    test([1,2,3,4,5,6,7,8,9,10], [ function(val, key, obj) {
        print('callback', key); if (count == 0) { return 0; }; count--; return 1;
    }]);

    // error in callback propagates outwards

    test([1,2,3], [ function(val, key, obj) {
        var e;
        print('callback', val);
        e = new Error('callback error');
        e.name = 'CallbackError';
        throw e;
    }]);

    // this binding, non-strict callbacks gets a coerced binding

    test([1,2,3], [ function(val, key, obj) {
        print('nonstrict', typeof this);
    }]);

    test([1,2,3], [ function(val, key, obj) {
        print('nonstrict', typeof this);
    }, null]);

    test([1,2,3], [ function(val, key, obj) {
        print('nonstrict', typeof this);
    }, 'foo']);

    test([1,2,3], [ function(val, key, obj) {
        'use strict';
        print('strict', typeof this, this);
    }]);

    test([1,2,3], [ function(val, key, obj) {
        'use strict';
        print('strict', typeof this, this);
    }, null]);  // Note: typeof null -> 'object'

    test([1,2,3], [ function(val, key, obj) {
        'use strict';
        print('strict', typeof this, this);
    }, 'foo']);
}

try {
    basicTest();
} catch (e) {
    print(e);
}

/*===
mutation
foo 0 foo,bar,quux
bar 1 foo,bar,quux,baz
quux 2 foo,bar,quux,baz
undefined undefined
foo 0 foo,bar,quux
quux 2 foo,,quux
undefined undefined
foo 0 [object Object]
bar 1 [object Object]
quux 2 [object Object]
undefined undefined
foo 0 [object Object]
quux 2 [object Object]
undefined undefined
===*/

print('mutation');

function mutationTest() {
    var obj;

    // added element not recognized

    obj = [ 'foo', 'bar', 'quux' ];
    test(obj, [ function (val, key, obj) {
        print(val, key, obj);
        obj[3] = 'baz';
    }]);

    // deleted element not processed

    obj = [ 'foo', 'bar', 'quux' ];
    test(obj, [ function (val, key, obj) {
        print(val, key, obj);
        delete obj[1];
    }]);

    // same for non-array

    obj = { '0': 'foo', '1': 'bar', '2': 'quux', '3': 'baz', length: 3 };
    test(obj, [ function (val, key, obj) {
        print(val, key, obj);
        obj[4] = 'quuux';
        obj.length = 10;
    }]);

    obj = { '0': 'foo', '1': 'bar', '2': 'quux', '3': 'baz', length: 3 };
    test(obj, [ function (val, key, obj) {
        print(val, key, obj);
        delete obj[3]; delete obj[1];
        obj.length = 0;
    }]);
}

try {
    mutationTest();
} catch (e) {
    print(e);
}

/*===
coercion
TypeError
TypeError
undefined undefined
undefined undefined
undefined undefined
object [object global] string f number 0 object foo
object [object global] string o number 1 object foo
object [object global] string o number 2 object foo
undefined undefined
object [object global] number 1 number 0 object 1,2,3
object [object global] number 2 number 1 object 1,2,3
object [object global] number 3 number 2 object 1,2,3
undefined undefined
undefined undefined
object [object global] string foo number 0 object [object Object]
object [object global] string bar number 1 object [object Object]
object [object global] string quux number 2 object [object Object]
undefined undefined
object [object global] string foo number 0 object [object Object]
object [object global] string bar number 1 object [object Object]
object [object global] string quux number 2 object [object Object]
undefined undefined
object [object global] string foo number 0 object [object Object]
object [object global] string bar number 1 object [object Object]
object [object global] string quux number 2 object [object Object]
object [object global] string baz number 3 object [object Object]
undefined undefined
length valueOf
object [object global] string foo number 0 object [object Object]
object [object global] string bar number 1 object [object Object]
object [object global] string quux number 2 object [object Object]
undefined undefined
length valueOf
TypeError
callback 1 0 1,2,3,4,5,6,7,8,9,10
callback 2 1 1,2,3,4,5,6,7,8,9,10
callback 3 2 1,2,3,4,5,6,7,8,9,10
callback 4 3 1,2,3,4,5,6,7,8,9,10
callback 5 4 1,2,3,4,5,6,7,8,9,10
callback 6 5 1,2,3,4,5,6,7,8,9,10
callback 7 6 1,2,3,4,5,6,7,8,9,10
callback 8 7 1,2,3,4,5,6,7,8,9,10
callback 9 8 1,2,3,4,5,6,7,8,9,10
callback 10 9 1,2,3,4,5,6,7,8,9,10
undefined undefined
===*/

print('coercion');

function coercionTest() {
    var obj;

    // this

    test(undefined, [ retUndef ]);
    test(null, [ retUndef ]);
    test(true, [ retUndef ]);
    test(false, [ retUndef ]);
    test(123, [ retUndef ]);
    test('foo', [ retUndef ]);
    test([1,2,3], [ retUndef ]);
    test({ foo: 1, bar: 2 }, [ retUndef ]);

    // length

    obj = { '0': 'foo', '1': 'bar', '2': 'quux', '3': 'baz', '4': 'quux', length: '3.9' };
    test(obj, [ retUndef ]);
    obj = { '0': 'foo', '1': 'bar', '2': 'quux', '3': 'baz', '4': 'quux', length: 256*256*256*256 + 3.9 };  // coerces to 3
    test(obj, [ retUndef ]);
    obj = { '0': 'foo', '1': 'bar', '2': 'quux', '3': 'baz', '4': 'quux', length: -256*256*256*256 + 3.9 };  // coerces to 4
    test(obj, [ retUndef ]);

    obj = { '0': 'foo', '1': 'bar', '2': 'quux', 'length': {
        toString: function() {
            print('length toString');
            return 4;
        },
        valueOf: function() {
            print('length valueOf');
            return 3;
        }
    }};
    test(obj, [ retUndef ]);

    // callable check is done after length coercion

    obj = { '0': 'foo', '1': 'bar', '2': 'quux', 'length': {
        toString: function() {
            print('length toString');
            return 4;
        },
        valueOf: function() {
            print('length valueOf');
            return 3;
        }
    }};
    test(obj, [ null ]);

    // callback return value does not matter; no boolean coercion happens for forEach

    test([1,2,3,4,5,6,7,8,9,10], [ function (val, key, obj) {
        print('callback', val, key, obj);
        if (key == 0) { return 1.0; }  /*true*/
        else if (key == 1) { return 'foo'; }  /*true*/
        else if (key == 2) {
            // Note: object is always 'true', no coercion related calls are made
            return {
                toString: function() { print('callback retval toString'); return 0; },
                valueOf: function() { print('callback retval valueOf'); return key == 1 ? '' /*false*/ : 'foo' /*true*/; }
            };
        } else {
            return '';  /*false*/
        }
    } ]);
}

try {
    coercionTest();
} catch (e) {
    print(e);
}
