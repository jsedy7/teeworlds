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

function min(a, b)
{
    if(a < b) return a;
    return b;
}

function max(a, b)
{
    if(a > b) return a;
    return b;
}

function clamp(val, min, max)
{
    if(val < min) return min;
    if(val > max) return max;
    return val;
}

function distance(pos1, pos2)
{
    var deltaX = pos2.x - pos1.x;
    var deltaY = pos2.y - pos1.y;
    return Math.sqrt(deltaX*deltaX + deltaY*deltaY);
}

function vec2(x_, y_)
{
    return { x: x_, y: y_ };
}

function vec2Length(v)
{
    return Math.sqrt(v.x*v.x + v.y*v.y);
}

function vec2Normalize(v)
{
    var f = 1.0 / vec2Length(v);
    return vec2(v.x * f, v.y * f);
}

function vec2MulScalar(v, s)
{
    return vec2(v.x * s, v.y * s);
}

function vec2Sub(v1, v2)
{
    return vec2(v1.x - v2.x, v1.y - v2.y);
}

function vec2Angle(v)
{
    return Math.atan2(v.y, v.x);
}

function vec2Rotate(v, angle)
{
    const c = Math.cos(angle);
    const s = Math.sin(angle);
    return vec2(c * v.x - s * v.y, s * v.x + c * v.y);
}

function sign(f)
{
    if(f < 0) return -1;
    return 1;
}