#ifndef T_ROLL_ID_RECORD_STORE
#define T_ROLL_ID_RECORD_STORE

#include <QString>
#include <QFile>
#include <QLabel>
#include <QTime>
#include <QSettings>
#include <QDir>

#define RECORD_PATTERN_LOG  "log%1.txt"
#define RECORD_PATTERN_IMG  "pic%1_%2.bmp"
#define RECORD_PATTERN_CNF  "log.cnf"

class t_record_storage {

private:
    QString m_storage_path;
    QSettings m_cnf;
    int m_history;
    int m_counter;
    int m_image;

public:
    t_record_storage(QString storage_path, int history = 100):
        m_storage_path(storage_path),
        m_cnf(m_storage_path + "/" + RECORD_PATTERN_CNF, QSettings::IniFormat),
        m_history(history)
    {
        qDebug() << "t_record_storage::t_record_storage()";

        m_image = m_counter = 0;

        QDir dir(m_storage_path);
        if (!dir.exists()){

          qDebug() << "t_record_storage::t_record_storage(): mkdir" << m_storage_path;
          dir.mkdir(".");
        }

        m_counter = m_cnf.value("LAST", m_counter).toInt();
        m_history = m_cnf.value("HISTORY", m_history).toInt();

        qDebug() << "t_record_storage::t_record_storage(): pop" << "LAST =" << m_counter;
        qDebug() << "t_record_storage::t_record_storage(): pop" << "HISTORY =" << m_history;
    }

    ~t_record_storage()
    {
        m_cnf.setValue("LAST", m_counter);
        m_cnf.setValue("HISTORY", m_history);
        m_cnf.sync();

        qDebug() << "t_record_storage::~t_record_storage(): push back" << "LAST =" << m_counter;
        qDebug() << "t_record_storage::~t_record_storage(): push back" << "HISTORY =" << m_history;
    }

    void increment(){

        m_counter = (m_counter + 1) % m_history;
        qDebug() << "t_record_storage::increment()" << m_counter;

        QString log_path = QString(RECORD_PATTERN_LOG).arg(m_counter);
        if(QFile::remove(m_storage_path + "/" + log_path))
            qDebug() << "t_record_storage::increment():" << log_path << "deleted!";

        for(int i_image = 0; i_image < 99; i_image++){

            QString img_path = QString(RECORD_PATTERN_IMG).arg(m_counter).arg(i_image);
            if(QFile::remove(m_storage_path + "/" + img_path) == false)
                break;

            qDebug() << "t_record_storage::increment():" << img_path << "deleted!";
        }

        m_image = 0;
    }

    void append(QString &log){

        QString stamp = QTime::currentTime().toString("hh:mm:ss.zzz");
        qDebug() << "t_record_storage::append():" << stamp << "\r\n" << log;

        QFile log_file(m_storage_path + "/" + QString(RECORD_PATTERN_LOG).arg(m_counter));
        log_file.open(QIODevice::Append | QIODevice::Text);
        log_file.write(stamp.toLatin1());
        log_file.write("\r\n");
        log_file.write(log.toLatin1());
        log_file.close();
    }

    void insert(QImage &img){

        qDebug() << "t_record_storage::insert()";
        if(!img.isNull()){

            QLabel vizual;
            vizual.setPixmap(QPixmap::fromImage(img));
            vizual.show();

            QString img_path = QString(RECORD_PATTERN_IMG).arg(m_counter).arg(m_image++);
            img.save(m_storage_path + "/" + img_path);
            qDebug() << "t_record_storage::insert():" << img_path;
        }
    }

    void add(QString &log, QImage &img){

        qDebug() << "t_record_storage::add()";
        increment();
        append(log);
        insert(img);
    }
};

#endif // T_ROLL_ID_RECORD_STORE

