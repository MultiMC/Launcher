/* Copyright 2015 MultiMC Contributors
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

#include <QAbstractListModel>
#include <type_traits>
#include <functional>
#include <memory>

class BaseAbstractCommonModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit BaseAbstractCommonModel(QObject *parent = nullptr);

	// begin QAbstractItemModel interface
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	// end QAbstractItemModel interface

	virtual int size() const = 0;
	virtual int entryCount() const = 0;

	virtual QVariant formatData(const int index, int role, const QVariant &data) const { return data; }
	virtual QVariant sanetizeData(const int index, int role, const QVariant &data) const { return data; }

protected:
	virtual QVariant get(const int index, const int entry, const int role) const = 0;
	virtual bool set(const int index, const int entry, const int role, const QVariant &value) = 0;
	virtual bool canSet(const int entry) const = 0;
	virtual QString entryTitle(const int entry) const = 0;

	void notifyAboutToAddObject(const int at);
	void notifyObjectAdded();
	void notifyAboutToRemoveObject(const int at);
	void notifyObjectRemoved();
	void notifyBeginReset();
	void notifyEndReset();
};

namespace Detail
{
/// Helper to be able to handle pointers and references the same way
template<typename Object>
struct dereferenceObject
{
	Object &operator()(Object &obj) { return obj; }
	const Object &operator()(const Object &obj) { return obj; }
};
template <typename Object>
struct dereferenceObject<Object *>
{
	Object &operator()(Object *obj) { return *obj; }
};

template<typename Type, typename EntrySetterType, typename EntryGetterType>
class AbstractCommonModelCommon : public BaseAbstractCommonModel
{
	// helper needed for some stuff
	using Object = typename std::remove_pointer<Type>::type;

	struct IEntry
	{
		virtual ~IEntry() {}
		virtual void set(EntrySetterType object, const QVariant &value) = 0;
		virtual QVariant get(EntryGetterType object) const = 0;
		virtual bool canSet() const = 0;
	};
	template<typename T, typename Lambda>
	struct LambdaEntry : public IEntry
	{
		using Getter = std::function<T(EntryGetterType)>;

		explicit LambdaEntry(Lambda getter)
			: m_getter(getter) {}

		void set(EntrySetterType object, const QVariant &value) override {}
		QVariant get(EntryGetterType object) const override
		{
			return QVariant::fromValue<T>(m_getter(object));
		}
		bool canSet() const override { return false; }

	private:
		Getter m_getter;
	};
	template<typename T>
	struct VariableEntry : public IEntry
	{
		typedef T (Object::*Member);

		explicit VariableEntry(Member member)
			: m_member(member) {}

		void set(EntrySetterType object, const QVariant &value) override
		{
			Detail::dereferenceObject<Type>()(object).*m_member = value.value<T>();
		}
		QVariant get(EntryGetterType object) const override
		{
			return QVariant::fromValue<T>(Detail::dereferenceObject<Type>()(object).*m_member);
		}
		bool canSet() const override { return true; }

	private:
		Member m_member;
	};
	template<typename T>
	struct FunctionEntry : public IEntry
	{
		typedef T (Object::*Getter)() const;
		typedef void (Object::*Setter)(T);

		explicit FunctionEntry(Getter getter, Setter setter)
			: m_getter(getter), m_setter(setter) {}

		void set(EntrySetterType object, const QVariant &value) override
		{
			(Detail::dereferenceObject<Type>()(object).*m_setter)(value.value<T>());
		}
		QVariant get(EntryGetterType object) const override
		{
			return QVariant::fromValue<T>((Detail::dereferenceObject<Type>()(object).*m_getter)());
		}
		bool canSet() const override { return !!m_setter; }

	private:
		Getter m_getter;
		Setter m_setter;
	};

	/// \internal Returns true if the given entry/role pair already exists
	bool hasEntryRolePair(const int entry, const int role) const
	{
		return m_entries.size() > entry && m_entries[entry].second.contains(role);
	}

	/// \internal Adds the entry to the list of entries
	void addEntryInternal(IEntry *e, const int entry, const int role)
	{
		// in debug, we fail hard
		Q_ASSERT_X(!hasEntryRolePair(entry, role),
				   "AbstractCommonModel::addEntry",
				   "This entry/role pair has already been set");
		// in release, we fail silently
		if (hasEntryRolePair(entry, role))
		{
			return;
		}

		if (m_entries.size() <= entry)
		{
			m_entries.resize(entry + 1);
		}
		m_entries[entry].second.insert(role, e);
	}

	/// \internal \reimp
	QVariant get(const int index, const int entry, const int role) const override
	{
		Q_ASSERT(entry < m_entries.size());
		if (!m_entries[entry].second.contains(role))
		{
			return QVariant();
		}
		return m_entries[entry].second.value(role)->get(m_objects.at(index));
	}
	/// \internal \reimp
	bool set(const int index, const int entry, const int role, const QVariant &value) override
	{
		Q_ASSERT(entry < m_entries.size());
		if (!m_entries[entry].second.contains(role))
		{
			return false;
		}
		IEntry *e = m_entries[entry].second.value(role);
		if (!e->canSet())
		{
			return false;
		}
		e->set(m_objects[index], value);
		return true;
	}
	/// \internal \reimp
	bool canSet(const int entry) const override
	{
		Q_ASSERT(entry < m_entries.size());
		if (!m_entries[entry].second.contains(Qt::EditRole))
		{
			return false;
		}
		IEntry *e = m_entries[entry].second.value(Qt::EditRole);
		return e->canSet();
	}

	/// \internal \reimp
	QString entryTitle(const int entry) const override
	{
		return m_entries.at(entry).first;
	}

	/// The list of columns (in vertical mode). Each entry has the title (header) and a list of entries for each role
	QVector<QPair<QString, QMap<int, IEntry *>>> m_entries;
	/// The list of all rows (in vertical mode)
	QList<Type> m_objects;

protected:
	explicit AbstractCommonModelCommon()
		: BaseAbstractCommonModel() {}

	/// Can be reimplemented. Will be called by setAll after emitting aboutToReset.
	virtual void cleanup() = 0;

	void setEntryTitle(const int entry, const QString &title)
	{
		if (m_entries.size() <= entry)
		{
			m_entries.resize(entry + 1);
		}
		m_entries[entry].first = title;
	}

	/// For usage on initial set up (for example loading from a file)
	void setAll(const QList<Type> objects)
	{
		notifyBeginReset();
		cleanup();
		m_objects = objects;
		notifyEndReset();
	}

	// Functions for registering columns (in vertical mode)/roles. Usage: addEntry<Type>(<column>, <role>, <accessors...>)
	template<typename T, typename Getter, typename Setter>
	typename std::enable_if<std::is_member_function_pointer<Getter>::value && std::is_member_function_pointer<Setter>::value, void>::type
	addEntry(const int entry, const int role, Getter getter, Setter setter)
	{
		addEntryInternal(new FunctionEntry<T>(getter, setter), entry, role);
	}
	template<typename T, typename Getter>
	typename std::enable_if<std::is_member_function_pointer<Getter>::value, void>::type
	addEntry(const int entry, const int role, Getter getter)
	{
		addEntryInternal(new FunctionEntry<T>(getter, nullptr), entry, role);
	}
	template<typename T>
	typename std::enable_if<std::is_member_object_pointer<T (Object::*)>::value, void>::type
	addEntry(const int entry, const int role, T (Object::*member))
	{
		addEntryInternal(new VariableEntry<T>(member), entry, role);
	}
	template<typename T, typename Lambda>
	typename std::enable_if<!std::is_member_function_pointer<Lambda>::value && !std::is_member_object_pointer<Lambda>::value, void>::type
	addEntry(const int entry, const int role, Lambda lambda)
	{
		addEntryInternal(new LambdaEntry<T, Lambda>(lambda), entry, role);
	}

public:
	virtual ~AbstractCommonModelCommon() {}

	int size() const override { return m_objects.size(); }
	int entryCount() const override { return m_entries.size(); }

	void append(const Type &object)
	{
		notifyAboutToAddObject(size());
		m_objects.append(object);
		notifyObjectAdded();
	}
	void prepend(const Type &object)
	{
		notifyAboutToAddObject(0);
		m_objects.prepend(object);
		notifyObjectAdded();
	}
	void insert(const Type &object, const int index)
	{
		if (index >= size())
		{
			append(object);
		}
		else if (index <= 0)
		{
			prepend(object);
		}
		else
		{
			notifyAboutToAddObject(index);
			m_objects.insert(index, object);
			notifyObjectAdded();
		}
	}
	void remove(const int index)
	{
		notifyAboutToRemoveObject(index);
		m_objects.removeAt(index);
		notifyObjectRemoved();
	}
	Type get(const int index) const
	{
		return m_objects.at(index);
	}
	int find(Type obj) const
	{
		return m_objects.indexOf(obj);
	}
	QList<Type> getAll() const
	{
		return m_objects;
	}
};
}

template<typename Object>
class AbstractCommonModel : public Detail::AbstractCommonModelCommon<Object, Object &, const Object &>
{
	using SuperClass = Detail::AbstractCommonModelCommon<Object, Object &, const Object &>;

public:
	explicit AbstractCommonModel()
		: SuperClass() {}
	virtual ~AbstractCommonModel() {}

private:
	void cleanup() override {}
};
template<typename Object>
class AbstractCommonModel<Object *> : public Detail::AbstractCommonModelCommon<Object *, Object *, Object *>
{
	using SuperClass = Detail::AbstractCommonModelCommon<Object *, Object *, Object *>;

public:
	explicit AbstractCommonModel()
		: SuperClass() {}
	virtual ~AbstractCommonModel()
	{
		cleanup();
	}

private:
	void cleanup() override
	{
		return qDeleteAll(SuperClass::getAll());
	}
};
