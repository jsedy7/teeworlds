// common functions

function clone(obj)
{
    var objClone = {};
    for(var key in obj) {
        if(obj.hasOwnProperty(key)) {
            objClone[key] = obj[key];
        }
    }
    return objClone;
}

function copy(destObj, srcObj)
{
    for(var key in srcObj) {
        if(srcObj.hasOwnProperty(key)) {
            destObj[key] = srcObj[key];
        }
    }
}

function objToStr(obj)
{
    if(Array.isArray(obj)) {
        var str = "[ ";
        var arrayLength = obj.length;
        for (var i = 0; i < arrayLength; i++) {
            str += objToStr(obj[i]) + ", ";
        }
        str += "]";
        return str;
    }

    if((typeof obj === "object") && (obj !== null)) {
        var str = "{ ";
        for(var key in obj) {
            if(obj.hasOwnProperty(key)) {
                str += key + ": " + objToStr(obj[key]) + ", ";
            }
        }
        str += "}";
        return str;
    }

    return "" + obj;
}

function printObj(obj)
{
    print(objToStr(obj));
}

function printRawBuffer(buff)
{
    var str = "[";
    for(var i = 0; i < buff.length; i +=1) {
        str += "0x"+ buff[i].toString(16) + ",";
    }
    str += "]"
    print(str);
}

function mix(a, b, amount)
{
    return a + (b-a) * amount;
}

function fmod(a, b)
{
    return Number((a - (Math.floor(a / b) * b)).toPrecision(8));
}
