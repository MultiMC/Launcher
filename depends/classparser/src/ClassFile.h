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

#include <map>
#include <cstdint>
#include <vector>
#include <string>

#include "ConstantPool.h"
#include "Attribute.h"

namespace Java
{
class ClassFileException : public std::exception
{
public:
	ClassFileException(const std::string &what) : std::exception(), m_what(what) {}
	~ClassFileException() noexcept {}
	const char *what() const noexcept override { return m_what.data(); }
private:
	std::string m_what;
};
class NotFoundException : public ClassFileException
{
public:
	NotFoundException(const std::string &name) : ClassFileException("Couldn't find " + name) {}
};

typedef ClassFileException ClassFileReadException;
typedef NotFoundException FieldNotFoundException;
typedef NotFoundException MethodNotFoundException;

using namespace std;

enum class AccessFlag : uint16_t
{
	Public = 0x0001, // CMF
	Private = 0x0002, // MF
	Protected = 0x0004, // MF
	Static = 0x0008, // MF
	Final = 0x0010, // CMF
	Synchronized = 0x0020, // M
	Super = 0x0020, // C
	Bridge = 0x0040, // M
	Volatile = 0x0040, // F
	VarArgs = 0x0080, // M
	Transient = 0x0080, // F
	Native = 0x0100, // M
	Interface = 0x0200, // C
	Abstract = 0x0400, // CM
	Strict = 0x0800, // M
	Synthetic = 0x1000, // CMF
	Annotation = 0x2000, // C
	Enum = 0x4000 // CF
};
Q_DECLARE_FLAGS(AccessFlags, AccessFlag)

class FieldInfo
{
public:
	FieldInfo(QDataStream &stream, ConstantPool *pool);

	AccessFlags flags() const { return m_flags; }
	std::string name() const { return m_pool->at(m_nameIndex)->getAs<Utf8Constant>()->string(); }
	std::string descriptor() const { return m_pool->at(m_descriptorIndex)->getAs<Utf8Constant>()->string(); }
	std::vector<AbstractAttribute> attributes() const { return m_attributes; }

private:
	ConstantPool *m_pool;
	AccessFlags m_flags;
	uint16_t m_nameIndex, m_descriptorIndex;
	std::vector<AbstractAttribute> m_attributes;
};
class MethodInfo
{
public:
	MethodInfo(QDataStream &stream, ConstantPool *pool);

	AccessFlags flags() const { return m_flags; }
	std::string name() const { return m_pool->at(m_nameIndex)->getAs<Utf8Constant>()->string(); }
	std::string descriptor() const { return m_pool->at(m_descriptorIndex)->getAs<Utf8Constant>()->string(); }
	std::vector<AbstractAttribute> attributes() const { return m_attributes; }

private:
	ConstantPool *m_pool;
	AccessFlags m_flags;
	uint16_t m_nameIndex, m_descriptorIndex;
	std::vector<AbstractAttribute> m_attributes;
};

class ClassFile
{
public:
	ClassFile(QDataStream &stream);

	int minorVersion() const { return m_minorVersion; }
	int majorVersion() const { return m_majorVersion; }
	ConstantPool *pool() const { return m_pool; }
	AccessFlags accessFlags() const { return m_accessFlags; }
	ClassConstant *thisClass() const { return m_pool->at(m_thisClass)->getAs<ClassConstant>(); }
	ClassConstant *superClass() const { return m_pool->at(m_superClass)->getAs<ClassConstant>(); }
	vector<ClassConstant *> interfaces() const;
	vector<FieldInfo> fields() const { return m_fields; }
	vector<MethodInfo> methods() const { return m_methods; }
	vector<AbstractAttribute> attributes() const { return m_attributes; }
	FieldInfo field(const std::string &name) const;
	MethodInfo method(const std::string &name) const;

private:
	uint16_t m_minorVersion;
	uint16_t m_majorVersion;
	ConstantPool *m_pool;
	AccessFlags m_accessFlags;
	uint16_t m_thisClass;
	uint16_t m_superClass;
	vector<uint16_t> m_interfaces;
	vector<FieldInfo> m_fields;
	vector<MethodInfo> m_methods;
	vector<AbstractAttribute> m_attributes;
};
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Java::AccessFlags)
