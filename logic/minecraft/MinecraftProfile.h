/* Copyright 2013-2015 MultiMC Contributors
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

#include <QString>
#include <QList>
#include <memory>

#include "Library.h"
#include "Package.h"
#include "Resources.h"

class ProfileStrategy;

// FIXME: needs to be not minecraft, push it up the abstraction ladder.
class MinecraftProfile : public QAbstractListModel
{
	Q_OBJECT
	friend class ProfileStrategy;

public:
	explicit MinecraftProfile(ProfileStrategy *strategy);

	void setStrategy(ProfileStrategy * strategy);
	ProfileStrategy *strategy();

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	// FIXME: kill this
	/// install more jar mods
	void installJarMods(QStringList selectedFiles);

	/// Can patch file # be removed?
	bool canRemove(const int index) const;

	enum MoveDirection { MoveUp, MoveDown };
	/// move patch file # up or down the list
	void move(const int index, const MoveDirection direction);

	/// remove patch file # - including files/records
	bool remove(const int index);

	/// remove patch file by id - including files/records
	bool remove(const QString id);

	/// reload all profile patches from storage, clear the profile and apply the patches
	void reload();

	/// clear the profile
	void clear();

	/// apply the patches
	void reapply();

public:
	/// get file ID of the patch file at #
	QString versionFileId(const int index) const;

	/// get the profile patch by id
	PackagePtr versionPatch(const QString &id);

	/// get the profile patch by index
	PackagePtr versionPatch(int index);

	/// save the current patch order
	void saveCurrentOrder() const;

public: /* only use in ProfileStrategy */
	/// Remove all the patches
	void clearPatches();

	/// Add the patch object to the internal list of patches
	void appendPatch(PackagePtr patch);

public: /* data */
	// FIXME: needs to be not minecraft
	Minecraft::Resources resources;

private:
	QList<PackagePtr> VersionPatches;
	ProfileStrategy *m_strategy = nullptr;
};
