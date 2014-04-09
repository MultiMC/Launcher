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
#include "ConstantPool.h"

#include <QDebug>

using namespace Java;

AbstractConstant *AbstractConstant::read(QDataStream &stream, ConstantPool *pool)
{
	uint8_t tag;
	stream >> tag;
	const Type type = (Type)tag;
	AbstractConstant *constant = 0;
	switch (type)
	{
	case Type::Class:
		constant = new ClassConstant(type, pool);
		break;
	case Type::FieldRef:
	case Type::MethodRef:
	case Type::InterfaceMethodRef:
		constant = new MethodFieldInterfaceRefConstant(type, pool);
		break;
	case Type::String:
		constant = new StringConstant(type, pool);
		break;
	case Type::Integer:
		constant = new IntegerConstant(type, pool);
		break;
	case Type::Float:
		constant = new FloatConstant(type, pool);
		break;
	case Type::Long:
		constant = new LongConstant(type, pool);
		break;
	case Type::Double:
		constant = new DoubleConstant(type, pool);
		break;
	case Type::NameAndType:
		constant = new NameAndTypeConstant(type, pool);
		break;
	case Type::Utf8:
		constant = new Utf8Constant(type, pool);
		break;
	case Type::MethodHandle:
		constant = new MethodHandleConstant(type, pool);
		break;
	case Type::MethodType:
		constant = new MethodTypeConstant(type, pool);
		break;
	case Type::InvokeDynamic:
		constant = new InvokeDynamicConstant(type, pool);
		break;
	default:
		throw ConstantPoolReadException(QObject::tr("Unknown tag: %1").arg(tag));
	}
	constant->readInternal(stream);
	return constant;
}

ConstantPool::ConstantPool(QDataStream &stream)
{
	uint16_t size;
	stream >> size;
	size -= 1; // why java, why?
	m_constants.reserve(size);
	for (int i = 0; i < size; ++i)
	{
		m_constants.push_back(AbstractConstant::read(stream, this));
	}
}

AbstractConstant *ConstantPool::operator[](const int index) const
{
	return at(index);
}
AbstractConstant *ConstantPool::at(const int index) const
{
	return m_constants.at(index - 1);
}

std::string Utf8Constant::string() const
{
	QString out;
	for (auto it = m_bytes.begin(); it != m_bytes.end(); ++it)
	{
		uint8_t byte = *it;
		if ((byte & 0b10000000) == 0)
		{
			out.append(QChar(byte));
		}
		else if ((byte & 0b11100000) == 0b11000000)
		{
			uint8_t x = byte;
			++it;
			uint8_t y = *it;
			uint16_t character = ((x & 0x1f) << 6) + (y & 0x3f);
			out.append(QChar(character));
		}
		else if ((byte & 0b11110000) == 0b11100000)
		{
			uint8_t x = byte;
			++it;
			uint8_t y = *it;
			++it;
			uint8_t z = *it;
			out.append(QChar((x & 0xf) << 12) + ((y & 0x3f) << 6) + (z & 0x3f));
		}
		else if (byte == 0b11101101)
		{
			++it;
			uint8_t v = *it;
			++it;
			uint8_t w = *it;
			++it;
			// always 0b11101101
			++it;
			uint8_t y = *it;
			++it;
			uint8_t z = *it;
			out.append(QChar(0x10000 + ((v & 0x0f) << 16) + ((w & 0x3f) << 10) +
							 ((y & 0x0f) << 6) + (z & 0x3f)));
		}
		else
		{
			// fail?
		}
	}
	return out.toStdString();
}
float FloatConstant::value() const
{
	if (m_raw == 0x7f800000)
	{
		return INFINITY;
	}
	else if (m_raw == 0xff800000)
	{
		return -INFINITY;
	}
	else if ((m_raw >= 0x78f00001 && m_raw <= 0x7fffffff) ||
			 (m_raw >= 0xff800001 && m_raw <= 0xffffffff))
	{
		return NAN;
	}
	else
	{
		const int s = ((m_raw >> 31) == 0) ? 1 : -1;
		const int e = ((m_raw >> 23) & 0xff);
		const int m = (e == 0) ? (m_raw & 0x7fffff) << 1 : (m_raw & 0x7fffff) | 0x800000;
		return float(s) * float(m) * float(pow(2, e - 150));
	}
}
