#ifndef DEEZERDATABASE_H
#define DEEZERDATABASE_H

#include <QSqlDatabase>

/**
 * \brief       Deezer cache database
 * \author      Matthieu Bachelier
 * \copyright   GNU General Public License v3
 */
class DeezerDatabase : public QSqlDatabase
{
private:
	bool _isEmpty;

public:
	explicit DeezerDatabase();

	inline bool isEmpty() const { return _isEmpty; }
};

#endif // DEEZERDATABASE_H
