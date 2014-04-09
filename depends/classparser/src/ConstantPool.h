/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <QDataStream>
#include <exception>
#include <QDebug>

namespace Java
{
class AbstractConstant;

class ConstantPoolReadException : public std::exception
{
public:
	ConstantPoolReadException(const QString &what) : std::exception(), m_what(what) {}
	~ConstantPoolReadException() noexcept {}
	const char *what() const noexcept override { return qPrintable(m_what); }
private:
	QString m_what;
};

class ConstantPool
{
public:
	ConstantPool(QDataStream &stream);

	std::vector<AbstractConstant *> constants() const { return m_constants; }
	unsigned int size() const { return m_constants.size(); }
	AbstractConstant *operator[](const int index) const;
	AbstractConstant *at(const int index) const;

private:
	std::vector<AbstractConstant *> m_constants;
};

#define ConstantConstructor(NAME) NAME(const Type type, ConstantPool *pool) : AbstractConstant(type, pool) {}
struct AbstractConstant
{
	enum class Type : uint8_t
	{
		Class = 7,
		FieldRef = 9,
		MethodRef = 10,
		InterfaceMethodRef = 11,
		String = 8,
		Integer = 3,
		Float = 4,
		Long = 5,
		Double = 6,
		NameAndType = 12,
		Utf8 = 1,
		MethodHandle = 15,
		MethodType = 16,
		InvokeDynamic = 18
	};
	AbstractConstant(const Type type, ConstantPool *pool) : type(type), m_pool(pool) {}
	Type type;

	static AbstractConstant *read(QDataStream &stream, ConstantPool *pool);

	virtual QString toString() const { return "AbstractConstant"; }

	template<typename T>
	T *getAs() { return static_cast<T *>(this); }

protected:
	ConstantPool *m_pool;
	virtual void readInternal(QDataStream &stream) = 0;
};
struct Utf8Constant : public AbstractConstant
{
	ConstantConstructor(Utf8Constant)
	std::string string() const;
	void readInternal(QDataStream &stream)
	{
		uint16_t length;
		stream >> length;
		m_bytes.resize(length);
		for (int i = 0; i < length; ++i)
		{
			stream >> m_bytes[i];
		}
	}

	QString toString() const override { return QString("Utf8Constant: %1").arg(QString::fromStdString(string())); }

private:
	std::vector<uint8_t> m_bytes;
};
struct ClassConstant : public AbstractConstant
{
	ConstantConstructor(ClassConstant)
	std::string name() const { return m_pool->at(m_nameIndex)->getAs<Utf8Constant>()->string(); }
	void readInternal(QDataStream &stream)
	{
		stream >> m_nameIndex;
	}

	QString toString() const override { return "ClassConstant: " + QString::number(m_nameIndex); }

private:
	uint16_t m_nameIndex;
};
struct MethodFieldInterfaceRefConstant : public AbstractConstant
{
	ConstantConstructor(MethodFieldInterfaceRefConstant)
	std::string clazz() const { return m_pool->at(m_classIndex)->getAs<Utf8Constant>()->string(); }
	std::string nameAndType() const { return m_pool->at(m_nameAndType)->getAs<Utf8Constant>()->string(); }
	void readInternal(QDataStream &stream)
	{
		stream >> m_classIndex >> m_nameAndType;
	}

	QString toString() const override { return QString("MethodFieldInterfaceRefConstant: %1, %2").arg(m_classIndex).arg(m_nameAndType); }

private:
	uint16_t m_classIndex, m_nameAndType;
};
struct StringConstant : public AbstractConstant
{
	ConstantConstructor(StringConstant)
	std::string string() const { return m_pool->at(m_stringIndex)->getAs<Utf8Constant>()->string(); }
	void readInternal(QDataStream &stream)
	{
		stream >> m_stringIndex;
	}

	QString toString() const override { return "StringConstant: " + QString::number(m_stringIndex); }

private:
	uint16_t m_stringIndex;
};
struct IntegerConstant : public AbstractConstant
{
	ConstantConstructor(IntegerConstant)
	int value() const { return m_integer; }
	void readInternal(QDataStream &stream)
	{
		stream >> m_integer;
	}

	QString toString() const override { return "IntegerConstant: " + QString::number(value()); }

private:
	uint32_t m_integer;
};
struct FloatConstant : public AbstractConstant
{
	ConstantConstructor(FloatConstant)
	float value() const;
	void readInternal(QDataStream &stream)
	{
		stream >> m_raw;
	}

	QString toString() const override { return "FloatConstant: " + QString::number(value()); }

private:
	uint32_t m_raw;
};
struct LongConstant : public AbstractConstant
{
	ConstantConstructor(LongConstant)
	long value() const { return ((long) m_high << 32) + m_low; }
	void readInternal(QDataStream &stream)
	{
		stream >> m_high >> m_low;
	}

	QString toString() const override { return "LongConstant: " + QString::number(value()); }

private:
	uint32_t m_high, m_low;
};
struct DoubleConstant : public AbstractConstant
{
	ConstantConstructor(DoubleConstant)
	double value() const { return (m_high << 32) + m_low; }
	void readInternal(QDataStream &stream)
	{
		// FIXME it's not as easy as this
		stream >> m_high >> m_low;
	}

	QString toString() const override { return "DoubleConstant: " + QString::number(value()); }

private:
	uint32_t m_high, m_low;
};
struct NameAndTypeConstant : public AbstractConstant
{
	ConstantConstructor(NameAndTypeConstant)
	std::string name() const { return m_pool->at(m_nameIndex)->getAs<Utf8Constant>()->string(); }
	std::string descriptor() const { return m_pool->at(m_descriptorIndex)->getAs<Utf8Constant>()->string(); }
	void readInternal(QDataStream &stream)
	{
		stream >> m_nameIndex >> m_descriptorIndex;
	}

	QString toString() const override { return QString("NameAndTypeConstant: %1, %2").arg(m_nameIndex).arg(m_descriptorIndex); }

private:
	uint16_t m_nameIndex, m_descriptorIndex;
};
struct MethodHandleConstant : public AbstractConstant
{
	ConstantConstructor(MethodHandleConstant)
	int referenceKind() const { return m_referenceKind; }
	std::string reference() const { return m_pool->at(m_referenceIndex)->getAs<Utf8Constant>()->string(); }
	void readInternal(QDataStream &stream)
	{
		stream >> m_referenceKind >> m_referenceIndex;
	}

	QString toString() const override { return QString("MethodHandleConstant: %1, %2").arg(m_referenceKind).arg(m_referenceIndex); }

private:
	uint8_t m_referenceKind;
	uint16_t m_referenceIndex;
};
struct MethodTypeConstant : public AbstractConstant
{
	ConstantConstructor(MethodTypeConstant)
	std::string descriptor() const { return m_pool->at(m_descriptorIndex)->getAs<Utf8Constant>()->string(); }
	void readInternal(QDataStream &stream)
	{
		stream >> m_descriptorIndex;
	}

	QString toString() const override { return QString("MethodTypeConstant: %1").arg(m_descriptorIndex); }

private:
	uint16_t m_descriptorIndex;
};
struct InvokeDynamicConstant : public AbstractConstant
{
	ConstantConstructor(InvokeDynamicConstant)
	uint16_t bootstrapMethodAttrIndex() const { return m_bootstrapMethodAttrIndex; }
	NameAndTypeConstant *nameAndType() const { return m_pool->at(m_nameAndTypeIndex)->getAs<NameAndTypeConstant>(); }
	void readInternal(QDataStream &stream)
	{
		stream >> m_bootstrapMethodAttrIndex >> m_nameAndTypeIndex;
	}

	QString toString() const override { return QString("InvokeDynamicConstant: %1, %2").arg(m_bootstrapMethodAttrIndex).arg(m_nameAndTypeIndex); }

private:
	uint16_t m_bootstrapMethodAttrIndex, m_nameAndTypeIndex;
};
#undef ConstantConstructor
}
