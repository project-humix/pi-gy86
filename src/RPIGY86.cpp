#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include <math.h>

#include "RPIGY86.h"
#include "MPU6050.h"
#include "HMC5883L.h"
#include "MS5611.h"

using namespace v8;

#define FUNCTION_TEMPLATE_CLASS "RPiGY86"

v8::Eternal<v8::Function> RPIGY86::sFunction;

//not able to directly set these offset into HMC5883L
//therefore, keep a local offsets here.
int32_t gMagXOffset = 0, gMagYOffset = 0, gMagZOffset = 0;

float gMagGainTable[] = { 0.73f, 0.92f, 1.22f, 1.52f, 2.27f, 2.56f, 3.03f, 4.35f };

//GYRO_FS_250 : 0,  //  131 LSB/°/s
//GYRO_FS_500 : 1,  //  65.5 LSB/°/s
//GYRO_FS_1000: 2,  //  32.8 LSB/°/s
//GYRO_FS_2000: 3 };//  16.4 LSB/°/s
float gGryoScaleTable[] = { 131, 65.5f, 32.8f, 16.4f };

//FS_2   : 0,  //  16384 LSB/g
//FS_4   : 1,  //   8192 LSB/g
//FS_8   : 2,  //   4096 LSB/g
//FS_16  : 3 } //   2048 LSB/g
int gAccelScaleTable[] = { 16384, 8192, 4096, 2048 };

/*static*/
void RPIGY86::V8New(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    if (args.IsConstructCall())
    {
        new RPIGY86(args);
        args.GetReturnValue().Set(args.This());
    }
    else
    {
        v8::Local<v8::Function> func = RPIGY86::sGetFunction();
        v8::Local<v8::Value> newArgs[args.Length()];
        for ( int i = 0; i < args.Length(); i ++ )
        {
            newArgs[i] = args[i];
        }
        v8::Local<v8::Object> obj = func->NewInstance(Nan::GetCurrentContext(), args.Length(), newArgs).ToLocalChecked();
        args.GetReturnValue().Set(obj);
    }
}

/*static*/
void
RPIGY86::sGetMotion6(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 0 )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: getMotion6()").ToLocalChecked()));
    }
    _this->getMotion6(args);
}
/*static*/ void
RPIGY86::sGetMotion9(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }

    if ( args.Length()  != 0 )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: getMotion9()").ToLocalChecked()));
    }
    _this->getMotion9(args);
}

/*static*/
void
RPIGY86::sSetGryoXOffset(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setGryoXOffset(offset)").ToLocalChecked()));
    }
    _this->setGryoXOffset(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}
/*static*/
void
RPIGY86::sSetGryoYOffset(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setGryoYOffset(offset)").ToLocalChecked()));
    }
    _this->setGryoYOffset(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}
/*static*/
void
RPIGY86::sSetGryoZOffset(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setGryoZOffset(offset)").ToLocalChecked()));
    }
    _this->setGryoZOffset(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}

/*static*/
void
RPIGY86::sSetAccelXOffset(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setAccelXOffset(offset)").ToLocalChecked()));
    }
    _this->setAccelXOffset(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}
/*static*/
void
RPIGY86::sSetAccelYOffset(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setAccelYOffset(offset)").ToLocalChecked()));
    }
    _this->setAccelYOffset(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}

/*static*/
void
RPIGY86::sSetAccelZOffset(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setAccelZOffset(offset)").ToLocalChecked()));
    }
    _this->setAccelZOffset(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}

/*static*/
void
RPIGY86::sSetGryoRangeScale(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setGryoRangeScale(scale)").ToLocalChecked()));
    }
    _this->setGryoRangeScale(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}

/*static*/
void
RPIGY86::sGetGryoRangeScale(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 0 )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: getGryoRangeScale()").ToLocalChecked()));
    }
    _this->getGryoRangeScale(args);
}

/*static*/
void
RPIGY86::sSetAccelRangeScale(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setAccelRangeScale(scale)").ToLocalChecked()));
    }
    _this->setAccelRangeScale(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}

/*static*/
void
RPIGY86::sGetAccelRangeScale(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 0 )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: getAccelRangeScale()").ToLocalChecked()));
    }
    _this->getAccelRangeScale(args);
}

/*static*/
void
RPIGY86::sCalibrateMPU6050(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 0 )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: calibrateMPU6050()").ToLocalChecked()));
    }
    _this->calibrateMPU6050(args);
}

/*static*/
void
RPIGY86::sGetHeadingXYZ(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 0 )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: getHeadingXYZ()").ToLocalChecked()));
    }
    _this->getHeadingXYZ(args);
}

/*static*/
void
RPIGY86::sGetHeading(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 0 )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: getHeading()").ToLocalChecked()));
    }
    _this->getHeading(args);
}

/*static*/
void
RPIGY86::sSetMagXOffset(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setMagXOffset(offset)").ToLocalChecked()));
    }
    _this->setMagXOffset(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}
/*static*/
void
RPIGY86::sSetMagYOffset(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setMagYOffset(offset)").ToLocalChecked()));
    }
    _this->setMagYOffset(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}
/*static*/
void
RPIGY86::sSetMagZOffset(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setMagZOffset(offset)").ToLocalChecked()));
    }
    _this->setMagZOffset(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}

/*static*/
void
RPIGY86::sSetMagGain(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 1 || !args[0]->IsNumber() )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: setMagGain(gain)").ToLocalChecked()));
    }
    _this->setMagGain(args[0]->ToNumber(Nan::GetCurrentContext()).ToLocalChecked()->Int32Value());
}

/*static*/
void
RPIGY86::sGetMagGain(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    RPIGY86* _this = RPIGY86::Unwrap<RPIGY86>(args.Holder());
    if ( !_this )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::ReferenceError(Nan::New("not a valid RPiGY86 object").ToLocalChecked()));
    }
    if ( args.Length()  != 0 )
    {
        args.GetIsolate()->ThrowException(
                v8::Exception::SyntaxError(Nan::New("usage: getMagGain()").ToLocalChecked()));
    }
    _this->getMagGain(args);
}
/*static*/
v8::Local<v8::Function>
RPIGY86::sGetFunction()
{
    Nan::EscapableHandleScope scope;
    v8::Isolate* isolate = v8::Isolate::GetCurrent();

    if ( sFunction.IsEmpty() ) {
        v8::Local<v8::FunctionTemplate> ftmpl = v8::FunctionTemplate::New(isolate, RPIGY86::V8New);
        ftmpl->SetClassName(Nan::New(FUNCTION_TEMPLATE_CLASS).ToLocalChecked());
        v8::Local<v8::ObjectTemplate> otmpl = ftmpl->InstanceTemplate();
        otmpl->SetInternalFieldCount(1);
        otmpl->Set(Nan::New("getMotion6").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sGetMotion6, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("getMotion9").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sGetMotion9, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setAccelXOffset").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetAccelXOffset, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setAccelYOffset").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetAccelYOffset, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setAccelZOffset").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetAccelZOffset, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setGryoXOffset").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetGryoXOffset, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setGryoYOffset").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetGryoYOffset, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setGryoZOffset").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetGryoZOffset, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setMagXOffset").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetMagXOffset, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setMagYOffset").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetMagYOffset, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setMagZOffset").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetMagZOffset, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setGryoRangeScale").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetGryoRangeScale, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("getGryoRangeScale").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sGetGryoRangeScale, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setAccelRangeScale").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetAccelRangeScale, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("getAccelRangeScale").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sGetAccelRangeScale, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("getMagGain").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sGetMagGain, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("setMagGain").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sSetMagGain, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("calibrateMPU6050").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sCalibrateMPU6050, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("getHeadingXYZ").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sGetHeadingXYZ, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        otmpl->Set(Nan::New("getHeading").ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, sGetHeading, v8::Local<v8::Value>(), v8::Signature::New(isolate, ftmpl)));
        sFunction.Set(isolate, ftmpl->GetFunction());
    }
    return scope.Escape(sFunction.Get(isolate));
}

RPIGY86::RPIGY86(const v8::FunctionCallbackInfo<v8::Value> &args)
    : mpu6050(nullptr), hmc5883l(nullptr), ms5611(nullptr)
{
    this->Wrap(args.This());
    initialize();
}

void RPIGY86::initialize()
{
    mpu6050 = new MPU6050();
    hmc5883l = new HMC5883L();
    ms5611 = new MS5611();
    mpu6050->initialize();
    mpu6050->setI2CBypassEnabled(true);
    hmc5883l->initialize();
    ms5611->begin();
}

RPIGY86::~RPIGY86()
{
    delete mpu6050;
    delete hmc5883l;
    delete ms5611;
}

void RPIGY86::getMotion6(const FunctionCallbackInfo<v8::Value> &args)
{
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    v8::Isolate* isolate = args.GetIsolate();
    mpu6050->getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    v8::Local<v8::Array> rev = v8::Array::New(isolate, 6);
    rev->Set(0, v8::Int32::New(isolate, ax));
    rev->Set(1, v8::Int32::New(isolate, ay));
    rev->Set(2, v8::Int32::New(isolate, az));
    rev->Set(3, v8::Int32::New(isolate, gx));
    rev->Set(4, v8::Int32::New(isolate, gy));
    rev->Set(5, v8::Int32::New(isolate, gz));
    args.GetReturnValue().Set(rev);
}

void RPIGY86::getMotion9(const FunctionCallbackInfo<v8::Value> &args)
{
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t mx, my, mz;

    v8::Isolate* isolate = args.GetIsolate();
    mpu6050->getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    v8::Local<v8::Array> rev = v8::Array::New(isolate, 9);
    rev->Set(0, v8::Int32::New(isolate, ax));
    rev->Set(1, v8::Int32::New(isolate, ay));
    rev->Set(2, v8::Int32::New(isolate, az));
    rev->Set(3, v8::Int32::New(isolate, gx));
    rev->Set(4, v8::Int32::New(isolate, gy));
    rev->Set(5, v8::Int32::New(isolate, gz));

    hmc5883l->getHeading(&mx, &my, &mz);
    rev->Set(6, v8::Int32::New(isolate, mx - gMagXOffset));
    rev->Set(7, v8::Int32::New(isolate, my - gMagYOffset));
    rev->Set(8, v8::Int32::New(isolate, mz));

    args.GetReturnValue().Set(rev);
}

void
RPIGY86::setGryoXOffset(int32_t offset)
{
    mpu6050->setXGyroOffset(offset);
}

void
RPIGY86::setGryoYOffset(int32_t offset)
{
    mpu6050->setYGyroOffset(offset);
}

void
RPIGY86::setGryoZOffset(int32_t offset)
{
    mpu6050->setZGyroOffset(offset);
}

void
RPIGY86::setAccelXOffset(int32_t offset)
{
    mpu6050->setXAccelOffset(offset);
}

void
RPIGY86::setAccelYOffset(int32_t offset)
{
    mpu6050->setYAccelOffset(offset);
}

void
RPIGY86::setAccelZOffset(int32_t offset)
{
    mpu6050->setZAccelOffset(offset);
}

void
RPIGY86::setGryoRangeScale(int32_t scale)
{
    mpu6050->setFullScaleGyroRange(scale);
}

void
RPIGY86::getGryoRangeScale(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    uint8_t scale = mpu6050->getFullScaleGyroRange();
    args.GetReturnValue().Set(v8::Uint32::New(args.GetIsolate(), scale));
}

void
RPIGY86::setAccelRangeScale(int32_t scale)
{
    mpu6050->setFullScaleAccelRange(scale);
}

void
RPIGY86::getAccelRangeScale(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    uint8_t scale = mpu6050->getFullScaleAccelRange();
    args.GetReturnValue().Set(v8::Uint32::New(args.GetIsolate(), scale));
}

void
RPIGY86::setMagXOffset(int32_t offset)
{
    gMagXOffset = offset;
}

void
RPIGY86::setMagYOffset(int32_t offset)
{
    gMagYOffset = offset;
}

void
RPIGY86::setMagZOffset(int32_t offset)
{
    gMagZOffset = offset;
}

void
RPIGY86::setMagGain(int32_t gain)
{
    hmc5883l->setGain(gain);
}

void
RPIGY86::getMagGain(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    uint8_t gain = hmc5883l->getGain();
    args.GetReturnValue().Set(v8::Uint32::New(args.GetIsolate(), gain));
}

void RPIGY86::measure(int* m_ax, int* m_ay, int* m_az, int* m_gx, int* m_gy,
        int* m_gz)
{
    int i = 0, buff_ax = 0, buff_ay = 0, buff_az = 0, buff_gx = 0, buff_gy = 0,
            buff_gz = 0;
    int16_t ax=0, ay=0, az=0, gx=0, gy=0,gz=0;
    static int buffersize = 1000;

    while (i < (buffersize + 101)) {
        // read raw accel/gyro measurements from device
        mpu6050->getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        if (i > 100 && i <= (buffersize + 100)) { //First 100 measures are discarded
            buff_ax += ax;
            buff_ay += ay;
            buff_az += az;
            buff_gx += gx;
            buff_gy += gy;
            buff_gz += gz;
        }
        i++;
        usleep(2); //Needed so we don't get repeated measures
    }

    *m_ax = buff_ax / buffersize;
    *m_ay = buff_ay / buffersize;
    *m_az = buff_az / buffersize;
    *m_gx = buff_gx / buffersize;
    *m_gy = buff_gy / buffersize;
    *m_gz = buff_gz / buffersize;
    printf("mean:%d, %d, %d, %d, %d, %d\n", *m_ax, *m_ay, *m_az, *m_gx,
            *m_gy, *m_gz);
}

void
RPIGY86::calibrateMPU6050(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    static int acel_deadzone=8;
    static int giro_deadzone=1;
    int ax_offset=0, ay_offset=0, az_offset=0,
         gx_offset=0, gy_offset=0, gz_offset=0;
    int mean_ax=0, mean_ay=0, mean_az=0, mean_gx=0, mean_gy=0, mean_gz=0;
    mpu6050->setXAccelOffset(ax_offset);
    mpu6050->setYAccelOffset(ay_offset);
    mpu6050->setZAccelOffset(az_offset);
    mpu6050->setXGyroOffset(gx_offset);
    mpu6050->setYGyroOffset(gy_offset);
    mpu6050->setZGyroOffset(gz_offset);
    usleep(1000);

    measure(&mean_ax, &mean_ay, &mean_az, &mean_gx, &mean_gy, &mean_gz);
    usleep(1000);
    ax_offset=-mean_ax/8;
    ay_offset=-mean_ay/8;
    az_offset=(16384-mean_az)/8;
    gx_offset=-mean_gx/4;
    gy_offset=-mean_gy/4;
    gz_offset=-mean_gz/4;

    while (1){
        int ready=0;
        mpu6050->setXAccelOffset(ax_offset);
        mpu6050->setYAccelOffset(ay_offset);
        mpu6050->setZAccelOffset(az_offset);
        mpu6050->setXGyroOffset(gx_offset);
        mpu6050->setYGyroOffset(gy_offset);
        mpu6050->setZGyroOffset(gz_offset);

        measure(&mean_ax, &mean_ay, &mean_az, &mean_gx, &mean_gy, &mean_gz);

        if (std::abs(mean_ax)<=acel_deadzone) ready++;
        else ax_offset=ax_offset-mean_ax/acel_deadzone;

        if (std::abs(mean_ay)<=acel_deadzone) ready++;
        else ay_offset=ay_offset-mean_ay/acel_deadzone;

        if (std::abs(16384-mean_az)<=acel_deadzone) ready++;
        else az_offset=az_offset+(16384-mean_az)/acel_deadzone;

        if (std::abs(mean_gx)<=giro_deadzone) ready++;
        else gx_offset=gx_offset-mean_gx/(giro_deadzone+1);

        if (std::abs(mean_gy)<=giro_deadzone) ready++;
        else gy_offset=gy_offset-mean_gy/(giro_deadzone+1);

        if (std::abs(mean_gz)<=giro_deadzone) ready++;
        else gz_offset=gz_offset-mean_gz/(giro_deadzone+1);

        if (ready==6) break;
        printf("ready:%d, %d, %d, %d, %d, %d, %d\n", ready, ax_offset, ay_offset,
           az_offset, gx_offset, gy_offset, gz_offset);
    }
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Array> rev = v8::Array::New(isolate, 6);
    rev->Set(0, v8::Int32::New(isolate, ax_offset));
    rev->Set(1, v8::Int32::New(isolate, ay_offset));
    rev->Set(2, v8::Int32::New(isolate, az_offset));
    rev->Set(3, v8::Int32::New(isolate, gx_offset));
    rev->Set(4, v8::Int32::New(isolate, gy_offset));
    rev->Set(5, v8::Int32::New(isolate, gz_offset));
    args.GetReturnValue().Set(rev);
}

void
RPIGY86::getHeadingXYZ(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    int16_t mx, my, mz;

    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Array> rev = v8::Array::New(isolate, 3);
    hmc5883l->getHeading(&mx, &my, &mz);
    rev->Set(0, v8::Int32::New(isolate, mx - gMagXOffset));
    rev->Set(1, v8::Int32::New(isolate, my - gMagYOffset));
    rev->Set(2, v8::Int32::New(isolate, mz));

    args.GetReturnValue().Set(rev);
}

void
RPIGY86::getHeading(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    int16_t mx, my, mz;
    //todo: get this value from somewhere
    static float declination = -4.28;
    uint8_t gain = hmc5883l->getGain();
    float scale = gMagGainTable[gain];
    hmc5883l->getHeading(&mx, &my, &mz);

    float myf = (my - gMagYOffset) * scale;
    float mxf = (mx - gMagXOffset) * scale;
    float heading = atan2(myf, mxf);
    if(heading < 0)
      heading += 2 * M_PI;
    heading = heading * 180/M_PI + declination;
    if ( heading < 0 ) {
        heading += 360;
    }

    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), heading));
}

NAN_MODULE_INIT(initialize) {
    Nan::HandleScope scope;
    Nan::Set(target, Nan::New(FUNCTION_TEMPLATE_CLASS).ToLocalChecked(),
            RPIGY86::sGetFunction());
}

NODE_MODULE(rpi_gy86, initialize)
