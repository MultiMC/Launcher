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
#include "Attribute.h"

using namespace Java;

AbstractAttribute::AbstractAttribute(QDataStream &stream, ConstantPool *pool) : m_pool(pool)
{
	uint32_t infoLength;
	stream >> m_nameIndex >> infoLength;
	m_info.resize(infoLength);
	for (int i = 0; i < infoLength; ++i)
	{
		stream >> m_info[i];
	}
}

AbstractAttribute *AbstractAttribute::resolved() const
{
	std::string name = this->name();
#define ATTRIBUTETYPECASE(ATTRIBUTE) if (name == #ATTRIBUTE) return new ATTRIBUTE ## Attribute(this, Type:: ATTRIBUTE)
	ATTRIBUTETYPECASE(ConstantValue);
	else ATTRIBUTETYPECASE(Code);
	else ATTRIBUTETYPECASE(StackMapTable);
	else ATTRIBUTETYPECASE(Exceptions);
	else ATTRIBUTETYPECASE(InnerClasses);
	else ATTRIBUTETYPECASE(EnclosingMethod);
	else ATTRIBUTETYPECASE(Synthetic);
	else ATTRIBUTETYPECASE(Signature);
	else ATTRIBUTETYPECASE(SourceFile);
	else ATTRIBUTETYPECASE(SourceDebugExtension);
	else ATTRIBUTETYPECASE(LineNumberTable);
	else ATTRIBUTETYPECASE(LocalVariableTable);
	else ATTRIBUTETYPECASE(LocalVariableTypeTable);
	else ATTRIBUTETYPECASE(Deprecated);
	else ATTRIBUTETYPECASE(RuntimeVisibleAnnotations);
	else ATTRIBUTETYPECASE(RuntimeInvisibleAnnotations);
	else ATTRIBUTETYPECASE(RuntimeVisibleParameterAnnotations);
	else ATTRIBUTETYPECASE(RuntimeInvisibleParameterAnnotations);
	else ATTRIBUTETYPECASE(AnnotationDefault);
	else ATTRIBUTETYPECASE(BootstrapMethods);
	else return 0;
#undef ATTRIBUTETYPECASE
}
