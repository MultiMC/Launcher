// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QAbstractListModel>
#include <type_traits>

class BaseAbstractCommonModel;

class CommonModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit CommonModel(const Qt::Orientation orientation, BaseAbstractCommonModel *backend, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
	friend class BaseAbstractCommonModel;
	void notifyAboutToAddObject(const int at);
	void notifyObjectAdded();
	void notifyAboutToRemoveObject(const int at);
	void notifyObjectRemoved();

	const Qt::Orientation m_orientation;
	BaseAbstractCommonModel *m_backend;
};

class BaseAbstractCommonModel
{
public:
	explicit BaseAbstractCommonModel(const Qt::Orientation orientation)
		: m_model(new CommonModel(orientation, this)) {}
	virtual ~BaseAbstractCommonModel() {}

	virtual int size() const = 0;
	virtual int entryCount() const = 0;

	virtual QVariant formatData(const int index, int role, const QVariant &data) const { return data; }
	virtual QVariant sanetizeData(const int index, int role, const QVariant &data) const { return data; }

	QAbstractItemModel *model() const { return m_model; }

protected:
	friend class CommonModel;
	virtual QVariant get(const int index, const int entry, const int role) const = 0;
	virtual bool set(const int index, const int entry, const int role, const QVariant &value) = 0;
	virtual bool canSet(const int entry) const = 0;
	virtual QString entryTitle(const int entry) const = 0;

	inline void notifyAboutToAddObject(const int at)
	{
		m_model->notifyAboutToAddObject(at);
	}
	inline void notifyObjectAdded()
	{
		m_model->notifyObjectAdded();
	}
	inline void notifyAboutToRemoveObject(const int at)
	{
		m_model->notifyAboutToRemoveObject(at);
	}
	inline void notifyObjectRemoved()
	{
		m_model->notifyObjectRemoved();
	}

	CommonModel *m_model;
};

template<typename Object>
class AbstractCommonModel : public BaseAbstractCommonModel
{
public:
	explicit AbstractCommonModel(const Qt::Orientation orientation)
		: BaseAbstractCommonModel(orientation) {}
	virtual ~AbstractCommonModel() {}

	int size() const override { return m_objects.size(); }
	int entryCount() const override { return m_entries.size(); }

	void append(const Object &object)
	{
		notifyAboutToAddObject(size());
		m_objects.append(object);
		notifyObjectAdded();
	}
	void prepend(const Object &object)
	{
		notifyAboutToAddObject(0);
		m_objects.prepend(object);
		notifyObjectAdded();
	}
	void insert(const Object &object, const int index)
	{
		if (index >= size())
		{
			prepend(object);
		}
		else if (index <= 0)
		{
			append(object);
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
	Object get(const int index) const
	{
		return m_objects.at(index);
	}

private:
	friend class CommonModel;
	QVariant get(const int index, const int entry, const int role) const override
	{
		if (m_entries.size() < entry || !m_entries[entry].second.contains(role))
		{
			return QVariant();
		}
		return m_entries[entry].second.value(role)->get(m_objects.at(index));
	}
	bool set(const int index, const int entry, const int role, const QVariant &value) override
	{
		if (m_entries.size() < entry || !m_entries[entry].second.contains(role))
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
	bool canSet(const int entry) const override
	{
		if (m_entries.size() < entry || !m_entries[entry].second.contains(Qt::EditRole))
		{
			return false;
		}
		IEntry *e = m_entries[entry].second.value(Qt::EditRole);
		return e->canSet();
	}

	QString entryTitle(const int entry) const override
	{
		return m_entries.at(entry).first;
	}

private:
	struct IEntry
	{
		virtual ~IEntry() {}
		virtual void set(Object &object, const QVariant &value) = 0;
		virtual QVariant get(const Object &object) const = 0;
		virtual bool canSet() const = 0;
	};
	template<typename T>
	struct VariableEntry : public IEntry
	{
		typedef T (Object::*Member);

		explicit VariableEntry(Member member)
			: m_member(member) {}

		void set(Object &object, const QVariant &value) override
		{
			object.*m_member = value.value<T>();
		}
		QVariant get(const Object &object) const override
		{
			return QVariant::fromValue<T>(object.*m_member);
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
			: m_getter(m_getter), m_setter(m_setter) {}

		void set(Object &object, const QVariant &value) override
		{
			object.*m_setter(value.value<T>());
		}
		QVariant get(const Object &object) const override
		{
			return QVariant::fromValue<T>(object.*m_getter());
		}
		bool canSet() const override { return !!m_setter; }

	private:
		Getter m_getter;
		Setter m_setter;
	};

	QList<Object> m_objects;
	QVector<QPair<QString, QMap<int, IEntry *>>> m_entries;

	void addEntryInternal(IEntry *e, const int entry, const int role)
	{
		if (m_entries.size() <= entry)
		{
			m_entries.resize(entry + 1);
		}
		m_entries[entry].second.insert(role, e);
	}

protected:
	template<typename Getter, typename Setter>
	typename std::enable_if<std::is_member_function_pointer<Getter>::value && std::is_member_function_pointer<Getter>::value, void>::type
	addEntry(Getter getter, Setter setter, const int entry, const int role)
	{
		addEntryInternal(new FunctionEntry<typename std::result_of<Getter>::type>(getter, setter), entry, role);
	}
	template<typename Getter>
	typename std::enable_if<std::is_member_function_pointer<Getter>::value, void>::type
	addEntry(Getter getter, const int entry, const int role)
	{
		addEntryInternal(new FunctionEntry<typename std::result_of<Getter>::type>(getter, nullptr), entry, role);
	}
	template<typename T>
	typename std::enable_if<!std::is_member_function_pointer<T (Object::*)>::value, void>::type
	addEntry(T (Object::*member), const int entry, const int role)
	{
		addEntryInternal(new VariableEntry<T>(member), entry, role);
	}

	void setEntryTitle(const int entry, const QString &title)
	{
		m_entries[entry].first = title;
	}
};
template<typename Object>
class AbstractCommonModel<Object *> : public BaseAbstractCommonModel
{
public:
	explicit AbstractCommonModel(const Qt::Orientation orientation)
		: BaseAbstractCommonModel(orientation) {}
	virtual ~AbstractCommonModel() {}

	int size() const override { return m_objects.size(); }
	int entryCount() const override { return m_entries.size(); }

	void append(Object *object)
	{
		notifyAboutToAddObject(size());
		m_objects.append(object);
		notifyObjectAdded();
	}
	void prepend(Object *object)
	{
		notifyAboutToAddObject(0);
		m_objects.prepend(object);
		notifyObjectAdded();
	}
	void insert(Object *object, const int index)
	{
		if (index >= size())
		{
			prepend(object);
		}
		else if (index <= 0)
		{
			append(object);
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
	Object *get(const int index) const
	{
		return m_objects.at(index);
	}

private:
	friend class CommonModel;
	QVariant get(const int index, const int entry, const int role) const override
	{
		if (m_entries.size() < entry || !m_entries[entry].second.contains(role))
		{
			return QVariant();
		}
		return m_entries[entry].second.value(role)->get(m_objects.at(index));
	}
	bool set(const int index, const int entry, const int role, const QVariant &value) override
	{
		if (m_entries.size() < entry || !m_entries[entry].second.contains(role))
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
	bool canSet(const int entry) const override
	{
		if (m_entries.size() < entry || !m_entries[entry].second.contains(Qt::EditRole))
		{
			return false;
		}
		IEntry *e = m_entries[entry].second.value(Qt::EditRole);
		return e->canSet();
	}

	QString entryTitle(const int entry) const override
	{
		return m_entries.at(entry).first;
	}

private:
	struct IEntry
	{
		virtual ~IEntry() {}
		virtual void set(Object *object, const QVariant &value) = 0;
		virtual QVariant get(Object *object) const = 0;
		virtual bool canSet() const = 0;
	};
	template<typename T>
	struct VariableEntry : public IEntry
	{
		typedef T (Object::*Member);

		explicit VariableEntry(Member member)
			: m_member(member) {}

		void set(Object *object, const QVariant &value) override
		{
			object->*m_member = value.value<T>();
		}
		QVariant get(Object *object) const override
		{
			return QVariant::fromValue<T>(object->*m_member);
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

		void set(Object *object, const QVariant &value) override
		{
			(object->*m_setter)(value.value<T>());
		}
		QVariant get(Object *object) const override
		{
			return QVariant::fromValue<T>((object->*m_getter)());
		}
		bool canSet() const override { return !!m_setter; }

	private:
		Getter m_getter;
		Setter m_setter;
	};

	QList<Object *> m_objects;
	QVector<QPair<QString, QMap<int, IEntry *>>> m_entries;

	void addEntryInternal(IEntry *e, const int entry, const int role)
	{
		if (m_entries.size() <= entry)
		{
			m_entries.resize(entry + 1);
		}
		m_entries[entry].second.insert(role, e);
	}

protected:
	template<typename Getter, typename Setter>
	typename std::enable_if<std::is_member_function_pointer<Getter>::value && std::is_member_function_pointer<Getter>::value, void>::type
	addEntry(Getter getter, Setter setter, const int entry, const int role)
	{
		addEntryInternal(new FunctionEntry<typename std::result_of<Getter>::type>(getter, setter), entry, role);
	}
	template<typename T>
	typename std::enable_if<std::is_member_function_pointer<typename FunctionEntry<T>::Getter>::value, void>::type
	addEntry(typename FunctionEntry<T>::Getter getter, const int entry, const int role)
	{
		addEntryInternal(new FunctionEntry<T>(getter, nullptr), entry, role);
	}
	template<typename T>
	typename std::enable_if<!std::is_member_function_pointer<T (Object::*)>::value, void>::type
	addEntry(T (Object::*member), const int entry, const int role)
	{
		addEntryInternal(new VariableEntry<T>(member), entry, role);
	}

	void setEntryTitle(const int entry, const QString &title)
	{
		m_entries[entry].first = title;
	}
};
