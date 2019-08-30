/*
 *  qmlsupporttest.cpp
 *
 *  This file is part of GammaRay, the Qt application inspection and
 *  manipulation tool.
 *
 *  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
 *  Author: Anton Kreuzkamp <anton.kreuzkamp@kdab.com>
 *
 *  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
 *  accordance with GammaRay Commercial License Agreement provided with the Software.
 *
 *  Contact info@kdab.com if any conditions of this licensing are not clear to you.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "baseprobetest.h"

#include <core/objectwrapper.h>

#include <QDebug>
#include <QtTest/qtest.h>
#include <QObject>
#include <QThread>
#include <QSignalSpy>


#include <QObject>
#include <QTimer>
#include <QVector>

#include <type_traits>
#include <memory>

using namespace GammaRay;


class SimpleNonQObjectTestObject {
public:
    explicit SimpleNonQObjectTestObject(int x, int y) : m_x(x), y(y) {}

    void setX(int x) { m_x = x; }

    int x() const { return m_x; }
    int y;

private:
    int m_x;
};

// class Foo {
//
//
//     using value_type = SimpleNonQObjectTestObject;                                                                                                           \
//     using ThisClass_t = Foo;
//
//     static constexpr int Flags = 0;
//
//     template<typename T1, typename T2>
//     static typename std::enable_if<std::is_same<T1, T2>::value>::type noop(T1 *t1) { return t1; }
//
//     template<typename T = value_type> /* Dummy parameter to make SFINAE applicable */
//     friend auto fetch_x(ThisClass_t *self, typename std::enable_if<std::is_same<T, value_type>::value && (Flags & LAMBDA_COMMAND) != 0>::type* = nullptr)
//     -> decltype(wrap<Flags>(x(static_cast<T*>(nullptr))))
//     {
//         return wrap<Flags>(x(noop<T, value_type>(self)->object));
//     }
//     template<typename PrivateClass>
//     friend auto fetch_x(ThisClass_t *self, typename std::enable_if<!std::is_same<PrivateClass, value_type>::value && (Flags & DptrMember) != 0>::type* = nullptr)
//     -> decltype(wrap<Flags>(std::declval<PrivateClass>().x()))
//     {
//         return wrap<Flags>(PrivateClass::get(self->object)->x());
//     }
//     template<typename T = value_type>
//     friend auto fetch_x(ThisClass_t *self, typename std::enable_if<std::is_same<T, value_type>::value && (Flags & Getter) != 0>::type* = nullptr)
//     -> decltype(wrap<Flags>(std::declval<T>().x()))
//     {
//         return wrap<Flags>(noop<T, value_type>(self)->object->x());
//     }
//     template<typename T = value_type>
//     friend auto fetch_x(ThisClass_t *self, typename std::enable_if<std::is_same<T, value_type>::value && (Flags & MemberVar) != 0>::type* = nullptr)
//     -> decltype(wrap<Flags>(std::declval<T>().x))
//     {
//         return wrap<Flags>(noop<T, value_type>(self)->object->x);
//     }
//
//     SimpleNonQObjectTestObject *object;
// };


class QObjectTestObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int x READ x WRITE setXX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setYY NOTIFY yChanged)
    Q_PROPERTY(QTimer *t MEMBER t)

signals:
    void xChanged();
    void yChanged();

public:
    QObjectTestObject(QObjectTestObject *parent = nullptr) : QObject(parent), t(new QTimer(this)), m_parent(parent) {}
    explicit QObjectTestObject(int x, int y, QObjectTestObject *parent = nullptr) : QObject(parent), t(new QTimer(this)), m_x(x), m_y(y), m_parent(parent) {}
    virtual ~QObjectTestObject() {}
    int x() const { return m_x; }
    int y() const { return m_y; }

    QObjectTestObject *parent() const { return m_parent; }

    void setXX(int x) { m_x = x; emit xChanged(); }
    void setYY(int y) { m_y = y; emit yChanged(); }

    QString str() { return QStringLiteral("Hello World"); }
    QString echo(const QString &s) const { return s; }

    QTimer *t = nullptr;

    QVector<QObjectTestObject *> children() const
    {
        return m_children;
    }

    QVector<QObjectTestObject *> m_children;

private:
    int m_x = 8;
    int m_y = 10;
    QObjectTestObject *m_parent = nullptr;
};

int getChildrenCount(const QObjectTestObject *obj)
{
    return obj->children().size();
}


class LinkedList {
public:
    LinkedList() = default;
    LinkedList(int i) : m_i(i) {}
    LinkedList(int i, LinkedList *next) : m_i(i), m_next(next) { next->m_prev = this; }
    ~LinkedList() { if(m_next) delete m_next; }

    int i() const { return m_i; }
    LinkedList *next() const { return m_next; }
    LinkedList *prev() const { return m_prev; }

private:
    int m_i;
    LinkedList *m_next = nullptr;
    LinkedList *m_prev = nullptr;
};


// class Worker : public QRunnable
// {
// public:
//     void run() override
//     {
//         newThreadObj.reset(new Test());
//         newThreadObj->setObjectName("newThreadObj");
// //         QObject::connect(newThreadObj.get(), &QObject::destroyed, mainThreadObj.get(), &QObject::deleteLater, Qt::DirectConnection);
//     }
//     std::unique_ptr<Test> newThreadObj;
//     std::unique_ptr<QObject> mainThreadObj;
// };

class DisabledCachingTestObject
{
public:
    int x() const
    {
        ++m_callCount;
        return m_x;
    }

    mutable int m_callCount = 0;
    int m_x = 42;
};

DECLARE_OBJECT_WRAPPER(SimpleNonQObjectTestObject,
                       PROP(x, Getter)
                       PROP(y, MemberVar)
)
DECLARE_OBJECT_WRAPPER(QTimer, PROP(isActive, Getter))
DECLARE_OBJECT_WRAPPER(QObjectTestObject,
                       PROP(x, Getter | QProp)
                       PROP(y, Getter | QProp)
                       PROP(str, NonConstGetter)
                       LAMBDA_PROP(halloDu, [](const QObjectTestObject *obj) { return obj->echo("Hello, you."); }, CustomCommand)
                       PROP(t, MemberVar | OwningPointer)
                       PROP(children, Getter | OwningPointer)
                       PROP(parent, Getter | NonOwningPointer)
                       LAMBDA_PROP(childrenCount, [](const QObjectTestObject *obj) { return getChildrenCount(obj); }, CustomCommand)
)
DECLARE_OBJECT_WRAPPER(LinkedList,
                       PROP(i, Getter)
                       PROP(prev, Getter | NonOwningPointer)
                       PROP(next, Getter | OwningPointer)
)
DECLARE_OBJECT_WRAPPER(DisabledCachingTestObject,
                       DISABLE_CACHING
                       PROP(x, Getter)
)
namespace GammaRay {

class ObjectWrapperTest : public BaseProbeTest
{
    Q_OBJECT
private slots:
    virtual void init()
    {
        createProbe();
    }

    void initTestCase()
    {
//         ObjectShadowDataRepository::instance()->m_objectToWrapperControlBlockMap.clear(); // FIXME this should not be necessary!
    }

    void testBasics()
    {
        QObjectTestObject t;
        t.m_children = QVector<QObjectTestObject *> { new QObjectTestObject {1, 2, &t}, new QObjectTestObject {3, 4, &t} };
        ObjectHandle<QObjectTestObject> w { &t };
        //     error<decltype(w->children())>();


        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y());
        QCOMPARE(w->str(), t.str());
        QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
        QCOMPARE(w->t()->isActive(), t.t->isActive());

        t.setXX(16);
        t.setYY(20);

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y());
        QCOMPARE(w->str(), t.str());
        QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
        QCOMPARE(w->t()->isActive(), t.t->isActive());

        static_assert(std::is_same<decltype(w->t()), ObjectHandle<QTimer>>::value,
                      "Something broke with the wrapping of objects into ObjectHandles...");
        static_assert(std::is_same<typename decltype(w->children())::value_type, ObjectHandle<QObjectTestObject >>::value,
                      "Something broke with the wrapping of object lists into lists of ObjectHandles...");

        for (auto x : w->children()) {
            QCOMPARE(x->parent()->m_control, w->m_control);
            QCOMPARE(x->x(), x.object()->x());
            QCOMPARE(x->y(), x.object()->y());
            QCOMPARE(x->str(), x.object()->str());
            QCOMPARE(x->halloDu(), QStringLiteral("Hello, you."));
            QCOMPARE(x->t()->isActive(), x.object()->t->isActive());
        }
    }

    void testCleanup()
    {
        ObjectShadowDataRepository::instance()->m_objectToWrapperControlBlockMap.clear();
        {
            QObjectTestObject t;
            t.m_children = QVector<QObjectTestObject *> { new QObjectTestObject {1, 2, &t}, new QObjectTestObject {3, 4, &t} };
            ObjectHandle<QObjectTestObject> w { &t };
            QCOMPARE(ObjectShadowDataRepository::instance()->m_objectToWrapperControlBlockMap.size(), 6); // test object with two children, every test object has a qtimer-child.
        }
        QCOMPARE(ObjectShadowDataRepository::instance()->m_objectToWrapperControlBlockMap.size(), 0);
    }

// SKIP: This feature has been removed. It's now simply forbidden (and asserted) to create an ObjectHandle from the wrong thread.
//     void testThreadBoundaries()
//     {
// //         auto task = std::unique_ptr<Worker>(new Worker());
// //         task->setAutoDelete(false);
// //         QThreadPool::globalInstance()->start(task.get());
//
//         ObjectShadowDataRepository::instance()->m_objectToWrapperControlBlockMap.clear(); // FIXME this should not be necessary!
//         QThread workerThread;
//         workerThread.start();
//         QObjectTestObject t;
//         t.m_children = QVector<QObjectTestObject *> { new QObjectTestObject {1, 2, &t}, new QObjectTestObject {3, 4, &t} };
//         t.moveToThread(&workerThread);
//
//         ObjectHandle<QObjectTestObject> w { &t };
//         //     error<decltype(w->children())>();
//
//         QCOMPARE(w->x(), t.x());
//         QCOMPARE(w->y(), t.y());
//         QCOMPARE(w->str(), t.str());
//         QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
//         QCOMPARE(w->t()->isActive(), t.t->isActive());
//
//         t.setXX(16);
//         t.setYY(20);
//
//         QCOMPARE(w->x(), t.x());
//         QCOMPARE(w->y(), t.y());
//         QCOMPARE(w->str(), t.str());
//         QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
//         QCOMPARE(w->t()->isActive(), t.t->isActive());
//
//         static_assert(std::is_same<typename decltype(w->children())::value_type, ObjectHandle<QObjectTestObject >>::value,
//                       "Something broke with the wrapping of object lists into lists of ObjectHandles...");
//
//         for (auto x : w->children()) {
//             QCOMPARE(x->parent()->m_control, w->m_control);
//             QCOMPARE(x->x(), x.object()->x());
//             QCOMPARE(x->y(), x.object()->y());
//             QCOMPARE(x->str(), x.object()->str());
//             QCOMPARE(x->halloDu(), QStringLiteral("Hello, you."));
//             QCOMPARE(x->t()->isActive(), x.object()->t->isActive());
//         }
//
//         workerThread.quit();
//         workerThread.wait();
//     }

    void testSelfReference()
    {
        ObjectShadowDataRepository::instance()->m_objectToWrapperControlBlockMap.clear();
        {
            LinkedList ll { 5, new LinkedList(6) };
            ObjectHandle<LinkedList> l { &ll };

            QCOMPARE(ObjectShadowDataRepository::instance()->m_objectToWrapperControlBlockMap.size(), 2);

            QVERIFY(l->object);
            QVERIFY(l->m_control);
            QVERIFY(ll.next()->prev());
            QVERIFY(l->next()->m_control != l->m_control);
            QVERIFY(l->next()->object);
            QVERIFY(l->next()->prev()->object);
            QCOMPARE(l->next()->prev()->m_control, l->m_control);
            QCOMPARE(l->next()->i(), ll.next()->i());
            QCOMPARE(l->next()->prev()->i(), ll.i());
            QCOMPARE(l->next()->prev()->next()->i(), ll.next()->i());
            QCOMPARE(l->next()->prev()->next()->prev()->i(), ll.i());
            QCOMPARE(l->next()->prev()->next()->prev()->next()->i(), ll.next()->i());
        }
        QCOMPARE(ObjectShadowDataRepository::instance()->m_objectToWrapperControlBlockMap.size(), 0);
    }

    void testMoveHandle()
    {
        QObjectTestObject t;
        auto wptr = new ObjectHandle<QObjectTestObject> { &t };
        ObjectHandle<QObjectTestObject> w = *wptr; // copy
        delete wptr;

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y());
        QCOMPARE(w->str(), t.str());
        QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
        QCOMPARE(w->t()->isActive(), t.t->isActive());

        t.setXX(16);
        t.setYY(20);

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y());
        QCOMPARE(w->str(), t.str());
        QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
        QCOMPARE(w->t()->isActive(), t.t->isActive());
    }

    void testNonQObject()
    {
        SimpleNonQObjectTestObject t {1, 2};
        ObjectHandle<SimpleNonQObjectTestObject> w { &t };

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y);

        t.setX(16);
        t.y = 20;

        w.refresh();

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y);
    }

    void testCachingDisabled()
    {
        DisabledCachingTestObject t;
        ObjectHandle<DisabledCachingTestObject> w { &t };

        QCOMPARE(t.m_callCount, 0);
        QCOMPARE(w->x(), 42);
        QCOMPARE(t.m_callCount, 1);

        t.m_x = 21;
        QCOMPARE(w->x(), 21);
        QCOMPARE(t.m_callCount, 2);
    }

    void testMetaObject()
    {
        SimpleNonQObjectTestObject t {1, 2};
        ObjectHandle<SimpleNonQObjectTestObject> w { &t };
        auto mo = ObjectHandle<SimpleNonQObjectTestObject>::staticMetaObject();

        QCOMPARE(mo->className(), QStringLiteral("SimpleNonQObjectTestObject"));
        QCOMPARE(mo->propertyCount(), 2);
        QCOMPARE(mo->propertyAt(0)->name(), "x");
        QCOMPARE(mo->propertyAt(0)->typeName(), "int");
        QCOMPARE(mo->propertyAt(0)->value(&*w), 1); // FIXME Fix the getter-API for accessing values through ObjectHandle-MetaObjects
        QCOMPARE(mo->propertyAt(1)->name(), "y");
        QCOMPARE(mo->propertyAt(1)->typeName(), "int");
        QCOMPARE(mo->propertyAt(1)->value(&*w), 2);

        t.setX(16);
        t.y = 20;

        QCOMPARE(mo->propertyAt(0)->value(&*w), 1);
        QCOMPARE(mo->propertyAt(1)->value(&*w), 2);
        w.refresh();
        QCOMPARE(mo->propertyAt(0)->value(&*w), 16);
        QCOMPARE(mo->propertyAt(1)->value(&*w), 20);

    }
};

}


QTEST_MAIN(GammaRay::ObjectWrapperTest)

#include "objectwrappertest.moc"
