#include "core/objectsystem.h"
#include "core/uri.h"

#include <mutex>

/*!
    \typedef Object::ObjectList

    Synonym for list<Object *>.
*/

/*!
    \typedef Object::LinkList

    Synonym for list<Link *>.
*/

inline bool operator==(const Object::Link &left, const Object::Link &right) {
    bool result = true;
    result &= (left.sender      == right.sender);
    result &= (left.receiver    == right.receiver);
    result &= (left.signal      == right.signal);
    result &= (left.method      == right.method);
    return result;
}

class ObjectPrivate {
public:
    ObjectPrivate() :
        m_pParent(nullptr),
        m_pCurrentSender(nullptr),
        m_UUID(0),
        m_Cloned(0) {

    }

    bool isLinkExist(const Object::Link &link) const {
        PROFILE_FUNCTION()
        for(const auto &it : m_lRecievers) {
            if(it == link) {
                return true;
            }
        }
        return false;
    }
    /// Parent object
    Object                         *m_pParent;
    /// Object name
    string                          m_sName;

    Object::ObjectList              m_mChildren;
    Object::LinkList                m_lRecievers;
    Object::LinkList                m_lSenders;

    Object                         *m_pCurrentSender;

    typedef queue<Event *>          EventQueue;
    EventQueue                      m_EventQueue;

    uint32_t                        m_UUID;
    uint32_t                        m_Cloned;

    mutex                           m_Mutex;
};
/*!
    \class Object
    \brief The Object class is the base calss for all object classes.
    \since Next 1.0
    \inmodule Core

    The object is the central part of the Next library.
    For communication between objects two mechanisms was implemented the signals and slots also event based approach.
    To connect two objects between use connect() method and the sender object  will notify the receiver object about necessary events.

    Objects can be organized into an object trees. Each object can have an unlimited number of children objects.
    When you assign parent to an object it automatically add itself to the parent children list. Parent object takes ownership of the child object.
    This means that the child will be automatically deleted if the parent object is deleted.
    Child object can be found in hierarchy of objects by path or by the type using find(), findChild() or findChildren().

    Each Object has name() and this name must be unique in space of object level in hierarchy i.e. parent with name "House" can't has two childs with name "Roof".
    This names is used to reach objects by its paths land.find("House/Roof") will return "Roof" object.

    Each Object has MetaObject declaration. MetaObject system can be used to declare and retrieve structure of object at runtime.

    Based on Actor model the object can't be copied only clone().

    \sa MetaObject
*/
/*!
    \fn T Object::findChild(bool recursive)

    Returns the first child of this object that can be cast to type T.
    The search is performed recursively, unless \a recursive option is false.

    Returns nullptr if no such object.

    \sa find(), findChildren()
*/
/*!
    \fn list<T> Object::findChildren(bool recursive)

    Returns all children of this object that can be cast to type T.
    The search is performed recursively, unless \a recursive option is false.

    Returns empty list if no such objects.

    \sa find(), findChildren()
*/
/*!
    \macro A_OBJECT(Class, Super)
    \relates Object

    This macro creates member functions to create MetaObject's.

    \a Class must be current class name
    \a Super must be class name of parent

    Example:
    \code
        class MyObject : public Object {
            A_OBJECT(MyObject, Object)
        };
    \endcode

    And then:
    \code
        MetaObject *meta    = MyObject::metaClass();
    \endcode
*/
/*!
    \macro A_REGISTER(Class, Super, Group)
    \relates Object

    This macro creates member functions for registering and unregistering in ObjectSystem factory.

    \a Class must be current class name
    \a Super must be class name of parent
    \a Group could be any and used to help manage factories

    Example:
    \code
        class MyObject : public Object {
            A_REGISTER(MyObject, Object, Core)
        };

        ....
        MyObject::registerClassFactory();
    \endcode

    And then:
    \code
        MyObject *oject = ObjectSystem::createObject<MyObject>();
    \endcode

    \note Also it's includes A_OBJECT() macro

    \sa ObjectSystem::objectCreate(), A_OBJECT()
*/
/*!
    \macro A_OVERRIDE(Class, Super, Group)
    \relates Object

    This macro works pertty mutch the same as A_REGISTER() macro but with little difference.
    It's override \a Super factory in ObjectSystem by own routine.
    And restore original state when do unregisterClassFactory().

    This macro can be used to implement polymorphic behavior for factories.

    \a Class must be current class name
    \a Super must be class name of parent
    \a Group could be any and used to help manage factories

    Example:
    \code
        class MyObject : public Object {
            A_OVERRIDE(MyObject, Object, Core)
        };

        ...
        MyObject::registerClassFactory();
    \endcode

    And then:
    \code
        Object *oject = ObjectSystem::createObject<Object>();
        MyObject *myObject  = dynamic_cast<MyObject *>(oject);
        if(myObject) {
            ...
        }
    \endcode

    \note Also it's includes A_OBJECT macro

    \sa ObjectSystem::objectCreate(), A_REGISTER(), A_OBJECT
*/
/*!
    \macro A_METHODS()
    \relates Object

    This macro is a container to keep information about included methods.

    There are three possible types of methods:
    \table
    \header
        \li Type
        \li Description
    \row
        \li A_SIGNAL
        \li Method without impelementation can't be invoked. Used for Signals and Slots mechanism.
    \row
        \li A_METHOD
        \li Standard method can be invoked. Used for general porposes.
    \row
        \li A_SLOT
        \li Very similar to A_METHOD but with special flag to be used for Signal-Slot mechanism.
    \endtable


    For example declare an introspectable class.
    \code
        class MyObject : public Object {
            A_REGISTER(MyObject, Object, General)

            A_METHODS(
                A_METHOD(foo)
                A_SIGNAL(signal)
            )

        public:
            void            foo             () { }

            void            signal          ();
        };
    \endcode

    And then:
    \code
        MyObject obj;
        const MetaObject *meta = obj.metaObject();

        int index   = meta->indexOfMethod("foo");
        if(index > -1) {
            MetaMethod method   = meta->method(index);
            if(method.isValid() {
                Variant value;
                method.invoke(&obj, value, 0, nullptr); // Will call MyObject::foo method
            }
        }
    \endcode
*/
/*!
    Constructs an object.

    By default Object create without parent to assign the parent object use setParent().
*/
Object::Object() :
        p_ptr(new ObjectPrivate) {
    PROFILE_FUNCTION()

    ObjectSystem::addObject(this);
}

Object::~Object() {
    ObjectSystem::removeObject(this);

    PROFILE_FUNCTION()
    {
        unique_lock<mutex> locker(p_ptr->m_Mutex);
        while(!p_ptr->m_EventQueue.empty()) {
            delete p_ptr->m_EventQueue.front();
            p_ptr->m_EventQueue.pop();
        }
    }

    while(!p_ptr->m_lSenders.empty()) {
        disconnect(p_ptr->m_lSenders.front().sender, 0, this, 0);
    }
    disconnect(this, 0, 0, 0);

    for(const auto &it : p_ptr->m_mChildren) {
        Object *c  = it;
        if(c) {
            c->p_ptr->m_pParent    = 0;
            delete c;
        }
    }
    p_ptr->m_mChildren.clear();

    if(p_ptr->m_pParent) {
        p_ptr->m_pParent->removeChild(this);
    }

    delete p_ptr;
}
/*!
    Returns new instance of Object class.
    This method is used in MetaObject system.

    \sa MetaObject
*/
Object *Object::construct() {
    PROFILE_FUNCTION()
    return new Object();
}
/*!
    Returns MetaObject and can be invoke without object of current class.
    This method is used in MetaObject system.

    \sa MetaObject
*/
const MetaObject *Object::metaClass() {
    PROFILE_FUNCTION()
    static const MetaObject staticMetaData("Object", nullptr, &construct, nullptr, nullptr);
    return &staticMetaData;
}
/*!
    Returns ponter MetaObject of this object.
    This method is used in MetaObject system.

    \sa MetaObject
*/
const MetaObject *Object::metaObject() const {
    PROFILE_FUNCTION()
    return Object::metaClass();
}
/*!
    Clones this object and set \a parent for the clone.
    Returns pointer to clone object.

    When you clone the Object or subclasses of it, all child objects also will be cloned.
    By default the parent for the new object will be nullptr. This clone will not have the name so you will need to set it manualy if required.

    Connections will be recreated with the same objects as original.

    \sa connect()
*/
Object *Object::clone(Object *parent) {
    PROFILE_FUNCTION()
    const MetaObject *meta  = metaObject();
    Object *result = meta->createInstance();
    result->setParent(parent);
    int count  = meta->propertyCount();
    for(int i = 0; i < count; i++) {
        MetaProperty lp = result->metaObject()->property(i);
        MetaProperty rp = meta->property(i);
        lp.write(result, rp.read(this));
    }
    for(auto it : getChildren()) {
        Object *clone  = it->clone(result);
        clone->setName(it->name());
    }
    for(auto it : p_ptr->m_lSenders) {
        MetaMethod signal  = it.sender->metaObject()->method(it.signal);
        MetaMethod method  = result->metaObject()->method(it.method);
        connect(it.sender, (to_string(1) + signal.signature()).c_str(),
                result, (to_string((method.type() == MetaMethod::Signal) ? 1 : 2) + method.signature()).c_str());
    }
    for(auto it : getReceivers()) {
        MetaMethod signal  = result->metaObject()->method(it.signal);
        MetaMethod method  = it.receiver->metaObject()->method(it.method);
        connect(result, (to_string(1) + signal.signature()).c_str(),
                it.receiver, (to_string((method.type() == MetaMethod::Signal) ? 1 : 2) + method.signature()).c_str());
    }
    result->p_ptr->m_Cloned = p_ptr->m_UUID;
    result->p_ptr->m_UUID   = ObjectSystem::generateUID();
    return result;
}
/*!
    Returns the UUID of cloned object.
*/
uint32_t Object::clonedFrom() const {
    PROFILE_FUNCTION()
    return p_ptr->m_Cloned;
}
/*!
    Returns a pointer to the parent object.
*/
Object *Object::parent() const {
    PROFILE_FUNCTION()
    return p_ptr->m_pParent;
}
/*!
    Returns name of the object.
*/
string Object::name() const {
    PROFILE_FUNCTION()
    return p_ptr->m_sName;
}
/*!
    Returns unique ID of the object.
*/
uint32_t Object::uuid() const {
    PROFILE_FUNCTION()
    return p_ptr->m_UUID;
}
/*!
    Returns class name the object.
*/
string Object::typeName() const {
    PROFILE_FUNCTION()
    return metaObject()->name();
}
/*!
    Creates connection beteen the \a signal of the \a sender and the \a method of the \a receiver.

    You must use the _SIGNAL() and _SLOT() macros when specifying \a signal and the \a method.
    \note The _SIGNAL() and _SLOT() must not contain any parameter values only parameter types.
    \code
        class MyObject : public Object {
            A_OVERRIDE(MyObject, Object, Core)

            A_METHODS(
                A_SLOT(onSignal),
                A_SIGNAL(signal)
            )
        public:
            void            signal          (bool value);

            void            onSignal        (bool value) {
                // Do some actions here
                ...
            }
        };
        ...
        MyObject obj1;
        MyObject obj2;

        Object::connect(&obj1, _SIGNAL(signal(bool)), &obj2, _SLOT(onSignal(bool)));
    \endcode
    \note Mehod signal in MyObject class may not have the implementation. It used only in description purposes in A_SIGNAL(signal) macros.

    Signal can also be conected to another signal.
    \code
        MyObject obj1;
        MyObject obj2;

        Object::connect(&obj1, _SIGNAL(signal(bool)), &obj2, _SIGNAL(signal(bool)));
    \endcode
*/
void Object::connect(Object *sender, const char *signal, Object *receiver, const char *method) {
    PROFILE_FUNCTION()
    if(sender && receiver) {
        int32_t snd = sender->metaObject()->indexOfSignal(&signal[1]);

        int32_t rcv;
        MetaMethod::MethodType right   = MetaMethod::MethodType(method[0] - 0x30);
        if(right == MetaMethod::Slot) {
            rcv = receiver->metaObject()->indexOfSlot(&method[1]);
        } else {
            rcv = receiver->metaObject()->indexOfSignal(&method[1]);
        }

        if(snd > -1 && rcv > -1) {
            Link link;

            link.sender     = sender;
            link.signal     = snd;
            link.receiver   = receiver;
            link.method     = rcv;

            if(!sender->p_ptr->isLinkExist(link)) {
                {
                    unique_lock<mutex> locker(sender->p_ptr->m_Mutex);
                    sender->p_ptr->m_lRecievers.push_back(link);
                }
                {
                    unique_lock<mutex> locker(receiver->p_ptr->m_Mutex);
                    receiver->p_ptr->m_lSenders.push_back(link);
                }
            }
        }
    }
}
/*!
    Disconnects \a signal in object \a sender from \a method in object \a receiver.

    A connection is removed when either of the objects are destroyed.

    disconnect() can be used in three ways:

    \list 1
    \li Disconnect everything from a specific sender:
        \code
            Object::disconnect(&obj1, 0, 0, 0);
        \endcode
    \li Disconnect everything connected to a specific signal:
        \code
            Object::disconnect(&obj1, _SIGNAL(signal(bool)), 0, 0);
        \endcode
    \li Disconnect all connections from the receiver:
        \code
            Object::disconnect(&obj1, 0, &obj3, 0);
        \endcode
    \endlist

    \sa connect()
*/
void Object::disconnect(Object *sender, const char *signal, Object *receiver, const char *method) {
    PROFILE_FUNCTION()
    if(sender && !sender->p_ptr->m_lRecievers.empty()) {
        for(auto snd = sender->p_ptr->m_lRecievers.begin(); snd != sender->p_ptr->m_lRecievers.end(); ) {
            Link *data = &(*snd);

            if(data->sender == sender) {
                if(signal == nullptr || data->signal == sender->metaObject()->indexOfMethod(&signal[1])) {
                    if(receiver == nullptr || data->receiver == receiver) {
                        if(method == nullptr || (receiver && data->method == receiver->metaObject()->indexOfMethod(&method[1]))) {

                            for(auto rcv = data->receiver->p_ptr->m_lSenders.begin(); rcv != data->receiver->p_ptr->m_lSenders.end(); ) {
                                if(*rcv == *data) {
                                    unique_lock<mutex> locker(data->receiver->p_ptr->m_Mutex);
                                    rcv = data->receiver->p_ptr->m_lSenders.erase(rcv);
                                } else {
                                    rcv++;
                                }
                            }
                            unique_lock<mutex> locker(sender->p_ptr->m_Mutex);
                            snd = sender->p_ptr->m_lRecievers.erase(snd);

                            continue;
                        }
                    }
                }
            }
            snd++;
        }
    }
}
/*!
    Marks this object to be deleted.
    This object will be deleted when event loop will call processEvent() method for this object.
*/
void Object::deleteLater() {
    PROFILE_FUNCTION()
    postEvent(new Event(Event::DESTROY));
}
/*!
    Returns list of child objects for this object.
*/
const Object::ObjectList &Object::getChildren() const {
    PROFILE_FUNCTION()
    return p_ptr->m_mChildren;
}
/*!
    Returns list of links to receivers objects for this object.
*/
const Object::LinkList &Object::getReceivers() const {
    PROFILE_FUNCTION()
    return p_ptr->m_lRecievers;
}

/*!
    Returns an object located along the \a path.

    \code
        Object obj1;
        Object obj2;

        obj1.setName("MainObject");
        obj2.setName("TestComponent2");
        obj2.setParent(&obj1);

        // result will contain pointer to obj2
        Object *result  = obj1.find("/MainObject/TestComponent2");
    \endcode

    Returns nullptr if no such object.

    \sa findChild()
*/
Object *Object::find(const string &path) {
    PROFILE_FUNCTION()
    if(p_ptr->m_pParent && path[0] == '/') {
        return p_ptr->m_pParent->find(path);
    }

    unsigned int start  = 0;
    if(path[0] == '/') {
        start   = 1;
    }
    int index  = path.find('/', 1);
    if(index > -1) {
        for(const auto &it : p_ptr->m_mChildren) {
            Object *o  = it->find(path.substr(index + 1));
            if(o) {
                return o;
            }
        }
    } else if(path.substr(start, index) == p_ptr->m_sName) {
        return this;
    }

    return nullptr;
}
/*!
    Makes the object a child of \a parent.

    \sa parent()
*/
void Object::setParent(Object *parent) {
    PROFILE_FUNCTION()
    if(p_ptr->m_pParent) {
        p_ptr->m_pParent->removeChild(this);
    }
    if(parent) {
        parent->addChild(this);
    }
    p_ptr->m_pParent    = parent;
}
/*!
    Set object name by provided \a name.

    \sa metaObject()
*/
void Object::setName(const string &name) {
    PROFILE_FUNCTION()
    if(!name.empty()) {
        p_ptr->m_sName = name;
    }
}

void Object::addChild(Object *value) {
    PROFILE_FUNCTION()
    if(value) {
        p_ptr->m_mChildren.push_back(value);
    }
}

void Object::removeChild(Object *value) {
    PROFILE_FUNCTION()
    auto it = p_ptr->m_mChildren.begin();
    while(it != p_ptr->m_mChildren.end()) {
        if(*it == value) {
            p_ptr->m_mChildren.erase(it);
            return;
        }
        it++;
    }
}
/*!
    Send specific \a signal with \a args for all connected receivers.

    For now it places signal directly to receivers queues.
    In case of another signal connected as method this signal will be emitted immediately.

    \note Receiver should be in event loop to process incoming message.

    \sa connect()
*/
void Object::emitSignal(const char *signal, const Variant &args) {
    PROFILE_FUNCTION()
    int32_t index   = metaObject()->indexOfSignal(&signal[1]);
    for(auto &it : p_ptr->m_lRecievers) {
        Link *link  = &(it);
        if(link->signal == index) {
            const MetaMethod &method   = link->receiver->metaObject()->method(link->method);
            if(method.type() == MetaMethod::Signal) {
                link->receiver->emitSignal(string(char(method.type() + 0x30) + method.signature()).c_str(), args);
            } else {
                // Queued Connection
                link->receiver->postEvent(new MethodCallEvent(link->method, link->sender, args));
            }
        }
    }
}
/*!
    Place event to internal \a event queue to be processed in event loop.
*/
void Object::postEvent(Event *event) {
    PROFILE_FUNCTION()
    unique_lock<mutex> locker(p_ptr->m_Mutex);
    p_ptr->m_EventQueue.push(event);
}

void Object::processEvents() {
    PROFILE_FUNCTION()
    while(!p_ptr->m_EventQueue.empty()) {
        unique_lock<mutex> locker(p_ptr->m_Mutex);
        Event *e   = p_ptr->m_EventQueue.front();
        switch (e->type()) {
            case Event::METHODCALL: {
                MethodCallEvent *call   = reinterpret_cast<MethodCallEvent *>(e);
                p_ptr->m_pCurrentSender = call->sender();
                Variant result;
                metaObject()->method(call->method()).invoke(this, result, 1, call->args());
                p_ptr->m_pCurrentSender = nullptr;
            } break;
            case Event::DESTROY: {
                locker.unlock();
                delete this;
                return;
            } break;
            default: {
                event(e);
            } break;
        }
        delete e;
        p_ptr->m_EventQueue.pop();
    }
}
/*!
    Abstract event handler.
    Developers should reimplement this method to handle events manually.
    Returns true in case of \a event was handled otherwise return false.
*/
bool Object::event(Event *event) {
    PROFILE_FUNCTION()
    A_UNUSED(event);
    return false;
}

/*!
    This method allows to DESERIALIZE \a data which not present as A_PROPERTY() in object.
*/
void Object::loadUserData(const VariantMap &data) {
    A_UNUSED(data)
}
/*!
    This method allows to SERIALIZE data which not present as A_PROPERTY() in object.
    Returns serialized data as VariantList.
*/
VariantMap Object::saveUserData() const {
    return VariantMap();
}
/*!
    Returns true if the object can be serialized; otherwise returns false.
*/
bool Object::isSerializable() const {
    return true;
}
/*!
    Returns the value of the object's property by \a name.

    If property not found returns invalid Variant.
    Information of all properties which provided by this object can be found in MetaObject.

    \sa setProperty(), metaObject(), Variant::isValid()
*/
Variant Object::property(const char *name) const {
    PROFILE_FUNCTION()
    const MetaObject *meta  = metaObject();
    int index   = meta->indexOfProperty(name);
    if(index > -1) {
        return meta->property(index).read(this);
    }
    return Variant();
}
/*!
    Sets the property with \a name to \a value.

    If property not found do nothing.
    Property must be defined as A_PROPERTY().
    Information of all properties which provided by this object can be found in MetaObject.

    \sa property(), metaObject(), Variant::isValid()
*/
void Object::setProperty(const char *name, const Variant &value) {
    PROFILE_FUNCTION()
    const MetaObject *meta  = metaObject();
    int index   = meta->indexOfProperty(name);
    if(index > -1) {
        meta->property(index).write(this, value);
    }
}
/*!
    Returns object which sent signal.
    \note This method returns a valid object only in receiver slot otherwise it's return nullptr
*/
Object *Object::sender() const {
    PROFILE_FUNCTION()
    return p_ptr->m_pCurrentSender;
}

void Object::setUUID(uint32_t id) {
    PROFILE_FUNCTION()
    p_ptr->m_UUID   = id;
}
