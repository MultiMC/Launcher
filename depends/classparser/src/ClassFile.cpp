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
#include "ClassFile.h"

using namespace std;
using namespace Java;

ClassFile::ClassFile(QDataStream &stream)
{
	stream.setByteOrder(QDataStream::BigEndian);
	uint32_t magic;
	stream >> magic;
	if (magic != 0xCAFEBABE)
	{
		throw ClassFileReadException("Invalid magic");
	}
	stream >> m_minorVersion >> m_majorVersion;
	m_pool = new ConstantPool(stream);
	uint16_t flags;
	stream >> flags >> m_thisClass >> m_superClass;
	m_accessFlags = (AccessFlags)flags;
	// interfaces
	{
		uint16_t numInterfaces;
		stream >> numInterfaces;
		m_interfaces.resize(numInterfaces);
		for (int i = 0; i < numInterfaces; ++i)
		{
			stream >> m_interfaces[i];
		}
	}
	// fields
	{
		uint16_t numFields;
		stream >> numFields;
		m_fields.reserve(numFields);
		for (int i = 0; i < numFields; ++i)
		{
			m_fields.push_back(FieldInfo(stream, m_pool));
		}
	}
	// methods
	{
		uint16_t numMethods;
		stream >> numMethods;
		m_methods.reserve(numMethods);
		for (int i = 0; i < numMethods; ++i)
		{
			m_methods.push_back(MethodInfo(stream, m_pool));
		}
	}
	// attributes
	{
		uint16_t numAttributes;
		stream >> numAttributes;
		m_attributes.reserve(numAttributes);
		for (int i = 0; i < numAttributes; ++i)
		{
			m_attributes.push_back(AbstractAttribute(stream, m_pool));
		}
	}
}

vector<ClassConstant *> ClassFile::interfaces() const
{
	vector<ClassConstant *> out;
	for (auto interface : m_interfaces)
	{
		out.push_back(m_pool->at(interface)->getAs<ClassConstant>());
	}
	return out;
}

FieldInfo ClassFile::field(const string &name) const
{
	for (auto f : m_fields)
	{
		if (f.name() == name)
		{
			return f;
		}
	}
	throw FieldNotFoundException(name);
}
MethodInfo ClassFile::method(const string &name) const
{
	for (auto m : m_methods)
	{
		if (m.name() == name)
		{
			return m;
		}
	}
	throw MethodNotFoundException(name);
}

FieldInfo::FieldInfo(QDataStream &stream, ConstantPool *pool)
	: m_pool(pool)
{
	uint16_t flags, numAttributes;
	stream >> flags >> m_nameIndex >> m_descriptorIndex >> numAttributes;
	m_flags = (AccessFlags)flags;
	m_attributes.reserve(numAttributes);
	for (int i = 0; i < numAttributes; ++i)
	{
		m_attributes.push_back(AbstractAttribute(stream, m_pool));
	}
}
MethodInfo::MethodInfo(QDataStream &stream, ConstantPool *pool)
	: m_pool(pool)
{
	uint16_t flags, numAttributes;
	stream >> flags >> m_nameIndex >> m_descriptorIndex >> numAttributes;
	m_flags = (AccessFlags)flags;
	m_attributes.reserve(numAttributes);
	for (int i = 0; i < numAttributes; ++i)
	{
		m_attributes.push_back(AbstractAttribute(stream, m_pool));
	}
}
