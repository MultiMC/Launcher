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

#include <vector>
#include <QDataStream>

#include "ConstantPool.h"

namespace Java
{
struct AbstractAttribute
{
	enum class Type
	{
		None,
		ConstantValue,
		Code,
		StackMapTable,
		Exceptions,
		InnerClasses,
		EnclosingMethod,
		Synthetic,
		Signature,
		SourceFile,
		SourceDebugExtension,
		LineNumberTable,
		LocalVariableTable,
		LocalVariableTypeTable,
		Deprecated,
		RuntimeVisibleAnnotations,
		RuntimeInvisibleAnnotations,
		RuntimeVisibleParameterAnnotations,
		RuntimeInvisibleParameterAnnotations,
		AnnotationDefault,
		BootstrapMethods
	};
	Type type;

	AbstractAttribute(ConstantPool *pool = 0,
					  const std::vector<uint8_t> &info = std::vector<uint8_t>())
		: m_pool(pool), m_info(info)
	{
	}
	AbstractAttribute(QDataStream &stream, ConstantPool *pool);
	virtual ~AbstractAttribute()
	{
	}

	std::string name() const
	{
		return m_pool->at(m_nameIndex)->getAs<Utf8Constant>()->string();
	}

	inline ConstantPool *pool() const
	{
		return m_pool;
	}
	inline std::vector<uint8_t> rawInfo() const
	{
		return m_info;
	}

	virtual QString toString() const
	{
		return "AbstractAttribute";
	}

	AbstractAttribute *resolved() const;
	template <typename T> T *getAs()
	{
		return static_cast<T *>(this);
	}

	inline bool operator==(const AbstractAttribute &other) const
	{
		return m_pool == other.m_pool && m_nameIndex == other.m_nameIndex &&
			   m_info == other.m_info;
	}

protected:
	ConstantPool *m_pool;
	uint16_t m_nameIndex;
	std::vector<uint8_t> m_info;

	uint16_t twoByteFromInfo(const int index = 0)
	{
		return (m_info[index] << 8) + m_info[index + 1];
	}
};
struct ConstantValueAttribute : public AbstractAttribute
{
	ConstantValueAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
		m_valueIndex = twoByteFromInfo();
	}
	AbstractConstant *value() const
	{
		return m_pool->at(m_valueIndex);
	}

	QString toString() const override
	{
		return "ConstantValueAttribute: " + value()->toString();
	}

private:
	uint16_t m_valueIndex;
};
struct CodeAttribute : public AbstractAttribute
{
	CodeAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
		m_maxStack = twoByteFromInfo(0);
		m_maxLocal = twoByteFromInfo(2);
		uint32_t codeLength = (m_info[4] << 24) + (m_info[5] << 16) + (m_info[6] << 8) + m_info[7];
		m_code.reserve(codeLength);
		for (int i = 0; i < codeLength; ++i)
		{
			m_code.push_back(m_info[i + 8]);
		}
		uint64_t offset = 8 + codeLength;
		uint16_t exceptionTableLength = twoByteFromInfo(offset); offset += 2;
		m_exceptionTable.reserve(exceptionTableLength);
		for (int i = 0; i < exceptionTableLength; ++i)
		{
			Exception e;
			e.startPC = twoByteFromInfo(offset);
			e.endPC = twoByteFromInfo(offset + 2);
			e.handlerPC = twoByteFromInfo(offset + 4);
			e.catchType = twoByteFromInfo(offset + 6);
			m_exceptionTable.push_back(e);
		}
		offset += exceptionTableLength * 8;
		uint16_t attributesLength = twoByteFromInfo(offset); offset += 2;
		for (int i = 0; i < attributesLength; ++i)
		{
			uint16_t length = 6 + (m_info[offset + 2] << 24) + (m_info[offset + 3] << 16) + (m_info[offset + 4] << 8) + m_info[offset + 5];
			QByteArray data;
			for (int i = 0; i < length; ++i)
			{
				data.append(m_info[offset + i]);
			}
			QDataStream stream(data);
			stream.setByteOrder(QDataStream::BigEndian);
			m_attributes.push_back(AbstractAttribute(stream, m_pool));
			offset += length;
		}
	}

	QString toString() const override
	{
		return QString("CodeAttribute: MaxStack: %1, MaxLocal: %2, Code size: %3, Exception "
					   "table size: %4, Attribute size: %5")
			.arg(m_maxStack)
			.arg(m_maxLocal)
			.arg(m_code.size())
			.arg(m_exceptionTable.size())
			.arg(m_attributes.size());
	}

	struct Exception
	{
		uint16_t startPC, endPC, handlerPC, catchType;
	};

	int maxStack() const
	{
		return m_maxStack;
	}
	int maxLocal() const
	{
		return m_maxLocal;
	}
	std::vector<uint8_t> code() const
	{
		return m_code;
	}
	std::vector<Exception> exceptionTable() const
	{
		return m_exceptionTable;
	}
	std::vector<AbstractAttribute> attributes() const
	{
		return m_attributes;
	}

private:
	uint16_t m_maxStack, m_maxLocal;
	std::vector<uint8_t> m_code;
	std::vector<Exception> m_exceptionTable;
	std::vector<AbstractAttribute> m_attributes;
};
struct StackMapTableAttribute : public AbstractAttribute
{
	StackMapTableAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "StackMapTableAttribute";
	}

private:
};
struct ExceptionsAttribute : public AbstractAttribute
{
	ExceptionsAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "ExceptionsAttribute";
	}

private:
};
struct InnerClassesAttribute : public AbstractAttribute
{
	InnerClassesAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "InnerClassesAttribute";
	}

private:
};
struct EnclosingMethodAttribute : public AbstractAttribute
{
	EnclosingMethodAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "EnclosingMethodAttribute";
	}

private:
};
struct SyntheticAttribute : public AbstractAttribute
{
	SyntheticAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "SyntheticAttribute";
	}

private:
};
struct SignatureAttribute : public AbstractAttribute
{
	SignatureAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
		m_signatureIndex = twoByteFromInfo();
	}
	std::string signature() const
	{
		return m_pool->at(m_signatureIndex)->getAs<Utf8Constant>()->string();
	}

	QString toString() const override
	{
		return "SignatureAttribute: " + QString::fromStdString(signature());
	}

private:
	uint16_t m_signatureIndex;
};
struct SourceFileAttribute : public AbstractAttribute
{
	SourceFileAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
		m_sourceFileIndex = twoByteFromInfo();
	}
	std::string sourceFile() const
	{
		return m_pool->at(m_sourceFileIndex)->getAs<Utf8Constant>()->string();
	}

	QString toString() const override
	{
		return "SourceFileAttribute: " + QString::fromStdString(sourceFile());
	}

private:
	uint16_t m_sourceFileIndex;
};
struct SourceDebugExtensionAttribute : public AbstractAttribute
{
	SourceDebugExtensionAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "SourceDebugExtensionAttribute";
	}

private:
};
struct LineNumberTableAttribute : public AbstractAttribute
{
	LineNumberTableAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "LineNumberTableAttribute";
	}

private:
};
struct LocalVariableTableAttribute : public AbstractAttribute
{
	LocalVariableTableAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "ConstantValueAttribute";
	}

private:
};
struct LocalVariableTypeTableAttribute : public AbstractAttribute
{
	LocalVariableTypeTableAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "ConstantValueAttribute";
	}

private:
};
struct DeprecatedAttribute : public AbstractAttribute
{
	DeprecatedAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "DeprecatedAttribute";
	}

private:
};
struct RuntimeVisibleAnnotationsAttribute : public AbstractAttribute
{
	RuntimeVisibleAnnotationsAttribute(const AbstractAttribute *abstractAttribute,
									   const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "RuntimeVisibleAnnotationsAttribute";
	}

private:
};
struct RuntimeInvisibleAnnotationsAttribute : public AbstractAttribute
{
	RuntimeInvisibleAnnotationsAttribute(const AbstractAttribute *abstractAttribute,
										 const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "RuntimeInvisibleAnnotationsAttribute";
	}

private:
};
struct RuntimeVisibleParameterAnnotationsAttribute : public AbstractAttribute
{
	RuntimeVisibleParameterAnnotationsAttribute(const AbstractAttribute *abstractAttribute,
												const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "RuntimeVisibleParameterAnnotationsAttribute";
	}

private:
};
struct RuntimeInvisibleParameterAnnotationsAttribute : public AbstractAttribute
{
	RuntimeInvisibleParameterAnnotationsAttribute(const AbstractAttribute *abstractAttribute,
												  const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "RuntimeInvisibleParameterAnnotationsAttribute";
	}

private:
};
struct AnnotationDefaultAttribute : public AbstractAttribute
{
	AnnotationDefaultAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "AnnotationDefaultAttribute";
	}

private:
};
struct BootstrapMethodsAttribute : public AbstractAttribute
{
	BootstrapMethodsAttribute(const AbstractAttribute *abstractAttribute, const Type type)
		: AbstractAttribute(abstractAttribute->pool(), abstractAttribute->rawInfo())
	{
		this->type = type;
	}

	QString toString() const override
	{
		return "BootstrapMethodsAttribute";
	}

private:
};
}
