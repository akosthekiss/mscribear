// Copyright (c) 2017 Akos Kiss.
//
// Licensed under the BSD 3-Clause License
// <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
// This file may not be copied, modified, or distributed except
// according to those terms.

// BluetoothRemoteGATTCharacteristic.readValue() in Servo returns
// Promise<string> instead of the standard-defined Promise<DataView>.
// This function ensures that in whatever format the value is read, its data is
// accessible via a getUint8(index) function.
function dataviewish(value) {
    if (typeof(value) == 'string') {
        return {
            getUint8: function(i) { return value.charCodeAt(i); },
        };
    }
    return value;
}
