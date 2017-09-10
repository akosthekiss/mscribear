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
