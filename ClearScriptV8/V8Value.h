// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

//-----------------------------------------------------------------------------
// V8BigInt
//-----------------------------------------------------------------------------

class V8BigInt final
{
public:

    V8BigInt(int signBit, std::vector<uint64_t>&& words):
        m_SignBit(signBit),
        m_Words(std::move(words))
    {
    }

    V8BigInt(const V8BigInt& that):
        m_SignBit(that.m_SignBit),
        m_Words(that.m_Words)
    {
    }

    V8BigInt(V8BigInt&& that) noexcept:
        m_SignBit(that.m_SignBit),
        m_Words(std::move(that.m_Words))
    {
    }

    const V8BigInt& operator=(const V8BigInt& that)
    {
        if (&that != this)
        {
            m_SignBit = that.m_SignBit;
            m_Words = that.m_Words;
        }

        return *this;
    }

    const V8BigInt& operator=(V8BigInt&& that) noexcept
    {
        if (&that != this)
        {
            m_SignBit = that.m_SignBit;
            m_Words = std::move(that.m_Words);
        }

        return *this;
    }

    int GetSignBit() const
    {
        return m_SignBit;
    }

    const std::vector<uint64_t>& GetWords() const
    {
        return m_Words;
    }

private:

    int m_SignBit;
    std::vector<uint64_t> m_Words;
};

//-----------------------------------------------------------------------------
// V8Value
//-----------------------------------------------------------------------------

class V8Value
{
public:

    enum NonexistentInitializer
    {
        Nonexistent
    };

    enum UndefinedInitializer
    {
        Undefined
    };

    enum NullInitializer
    {
        Null
    };

    enum DateTimeInitializer
    {
        DateTime
    };

    enum class Type: uint8_t
    {
        // IMPORTANT: maintain bitwise equivalence with managed enum V8.SplitProxy.V8Value.Type
        Nonexistent,
        Undefined,
        Null,
        Boolean,
        Number,
        String,
        DateTime,
        BigInt,
        V8Object,
        HostObject
    };

    enum class Subtype: uint8_t
    {
        // IMPORTANT: maintain bitwise equivalence with managed enum V8.SplitProxy.V8Value.Subtype
        None,
        Function,
        Iterator,
        Promise,
        Array,
        ArrayBuffer,
        DataView,
        Uint8Array,
        Uint8ClampedArray,
        Int8Array,
        Uint16Array,
        Int16Array,
        Uint32Array,
        Int32Array,
        BigUint64Array,
        BigInt64Array,
        Float32Array,
        Float64Array
    };

    enum class Flags : uint16_t
    {
        // IMPORTANT: maintain bitwise equivalence with managed enum V8.SplitProxy.V8Value.Flags
        None = 0,
        Shared = 0x0001,
        Fast = 0x0001,
        Async = 0x0002,
        Generator = 0x0004,
        Pending = 0x0008,
        Rejected = 0x0010
    };

    struct WireData;
    struct Decoded;
    struct FastArg;
    struct FastResult;

    explicit V8Value(NonexistentInitializer):
        m_Type(Type::Nonexistent)
    {
    }

    explicit V8Value(UndefinedInitializer):
        m_Type(Type::Undefined)
    {
    }

    explicit V8Value(NullInitializer):
        m_Type(Type::Null)
    {
    }

    explicit V8Value(bool value):
        m_Type(Type::Boolean)
    {
        m_Data.BooleanValue = value;
    }

    explicit V8Value(double value):
        m_Type(Type::Number)
    {
        m_Data.DoubleValue = value;
    }

    explicit V8Value(const StdString* pString):
        m_Type(Type::String)
    {
        m_Data.pString = pString;
    }

    V8Value(DateTimeInitializer, double value):
        m_Type(Type::DateTime)
    {
        m_Data.DoubleValue = value;
    }

    explicit V8Value(const V8BigInt* pBigInt):
        m_Type(Type::BigInt)
    {
        m_Data.pBigInt = pBigInt;
    }

    V8Value(V8ObjectHolder* pV8ObjectHolder, Subtype subtype, Flags flags):
        m_Type(Type::V8Object),
        m_Subtype(subtype),
        m_Flags(flags)
    {
        m_Data.pV8ObjectHolder = pV8ObjectHolder;
    }

    explicit V8Value(HostObjectHolder* pHostObjectHolder):
        m_Type(Type::HostObject),
        m_Subtype(::ToEnum<Subtype>(pHostObjectHolder->GetSubtype())),
        m_Flags(::ToEnum<Flags>(pHostObjectHolder->GetFlags()))
    {
        m_Data.pHostObjectHolder = pHostObjectHolder;
    }

    V8Value(const V8Value& that)
    {
        Copy(that);
    }

    V8Value(V8Value&& that) noexcept
    {
        Move(that);
    }

    explicit V8Value(const FastResult& result)
    {
        InitializeFromFastResult(result);
    }

    explicit V8Value(void*) = delete;

    const V8Value& operator=(const V8Value& that)
    {
        Dispose();
        Copy(that);
        return *this;
    }

    const V8Value& operator=(V8Value&& that) noexcept
    {
        Dispose();
        Move(that);
        return *this;
    }

    bool IsNonexistent() const
    {
        return m_Type == Type::Nonexistent;
    }

    bool IsUndefined() const
    {
        return m_Type == Type::Undefined;
    }

    bool IsNull() const
    {
        return m_Type == Type::Null;
    }

    bool AsBoolean(bool& result) const
    {
        if (m_Type == Type::Boolean)
        {
            result = m_Data.BooleanValue;
            return true;
        }

        return false;
    }

    bool AsNumber(double& result) const
    {
        if (m_Type == Type::Number)
        {
            result = m_Data.DoubleValue;
            return true;
        }

        return false;
    }

    bool AsString(const StdString*& pString) const
    {
        if (m_Type == Type::String)
        {
            pString = m_Data.pString;
            return true;
        }

        return false;
    }

    bool AsDateTime(double& result) const
    {
        if (m_Type == Type::DateTime)
        {
            result = m_Data.DoubleValue;
            return true;
        }

        return false;
    }

    bool AsBigInt(const V8BigInt*& pBigInt) const
    {
        if (m_Type == Type::BigInt)
        {
            pBigInt = m_Data.pBigInt;
            return true;
        }

        return false;
    }

    bool AsV8Object(V8ObjectHolder*& pV8ObjectHolder, Subtype& subtype, Flags& flags) const
    {
        if (m_Type == Type::V8Object)
        {
            pV8ObjectHolder = m_Data.pV8ObjectHolder;
            subtype = m_Subtype;
            flags = m_Flags;
            return true;
        }

        return false;
    }

    bool AsHostObject(HostObjectHolder*& pHostObjectHolder, Subtype& subtype, Flags& flags) const
    {
        if (m_Type == Type::HostObject)
        {
            pHostObjectHolder = m_Data.pHostObjectHolder;
            subtype = m_Subtype;
            flags = m_Flags;
            return true;
        }

        return false;
    }

    Type GetType() const
    {
        return m_Type;
    }

    void Decode(Decoded& decoded) const;

    ~V8Value()
    {
        Dispose();
    }

private:

    union Data
    {
        bool BooleanValue;
        double DoubleValue;
        const StdString* pString;
        V8ObjectHolder* pV8ObjectHolder;
        HostObjectHolder* pHostObjectHolder;
        const V8BigInt* pBigInt;
    };

    void Copy(const V8Value& that)
    {
        m_Type = that.m_Type;
        m_Subtype = that.m_Subtype;
        m_Flags = that.m_Flags;

        if (m_Type == Type::Boolean)
        {
            m_Data.BooleanValue = that.m_Data.BooleanValue;
        }
        else if (m_Type == Type::Number)
        {
            m_Data.DoubleValue = that.m_Data.DoubleValue;
        }
        else if (m_Type == Type::String)
        {
            m_Data.pString = new StdString(*that.m_Data.pString);
        }
        else if (m_Type == Type::DateTime)
        {
            m_Data.DoubleValue = that.m_Data.DoubleValue;
        }
        else if (m_Type == Type::BigInt)
        {
            m_Data.pBigInt = new V8BigInt(*that.m_Data.pBigInt);
        }
        else if (m_Type == Type::V8Object)
        {
            m_Data.pV8ObjectHolder = that.m_Data.pV8ObjectHolder->Clone();
        }
        else if (m_Type == Type::HostObject)
        {
            m_Data.pHostObjectHolder = that.m_Data.pHostObjectHolder->Clone();
        }
    }

    void Move(V8Value& that)
    {
        m_Type = that.m_Type;
        m_Subtype = that.m_Subtype;
        m_Flags = that.m_Flags;
        m_Data = that.m_Data;
        that.m_Type = Type::Nonexistent;
    }

    void InitializeFromFastResult(const FastResult& result);

    void Dispose()
    {
        if (m_Type == Type::String)
        {
            delete m_Data.pString;
        }
        else if (m_Type == Type::BigInt)
        {
            delete m_Data.pBigInt;
        }
        else if (m_Type == Type::V8Object)
        {
            delete m_Data.pV8ObjectHolder;
        }
        else if (m_Type == Type::HostObject)
        {
            delete m_Data.pHostObjectHolder;
        }
    }

    Type m_Type;
    Subtype m_Subtype;
    Flags m_Flags;
    int32_t m_Padding;
    Data m_Data;
};

static_assert(sizeof(V8Value) == 16, "The managed SplitProxy code assumes that sizeof(V8Value) is 16 on all platforms.");

//-----------------------------------------------------------------------------
// DefaultInitializedV8Value
//-----------------------------------------------------------------------------

template <typename TInitializer>
class DefaultInitializedV8Value final: public V8Value
{
public:

    using V8Value::V8Value;

    DefaultInitializedV8Value():
        V8Value(ms_Initializer)
    {
        static_assert(sizeof(DefaultInitializedV8Value) == sizeof(V8Value));
    }

    using V8Value::operator=;

private:

    static const TInitializer ms_Initializer {};
};

using NonexistentV8Value = DefaultInitializedV8Value<V8Value::NonexistentInitializer>;
using UndefinedV8Value = DefaultInitializedV8Value<V8Value::UndefinedInitializer>;
using NullV8Value = DefaultInitializedV8Value<V8Value::NullInitializer>;
