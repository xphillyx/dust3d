#include <QDebug>
#include <QElapsedTimer>
#include <QUuid>
#include "scriptrunner.h"

JSClassID ScriptRunner::js_partClassId = 0;
JSClassID ScriptRunner::js_componentClassId = 0;
JSClassID ScriptRunner::js_nodeClassId = 0;

static JSValue js_print(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    
    int i;
    const char *str;

    for (i = 0; i < argc; i++) {
        if (i != 0)
            runner->consoleLog() += ' ';
        str = JS_ToCString(context, argv[i]);
        if (!str)
            return JS_EXCEPTION;
        runner->consoleLog() += str;
        JS_FreeCString(context, str);
    }
    runner->consoleLog() += '\n';
    return JS_UNDEFINED;
}

static JSValue js_setAttribute(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 3) {
        runner->consoleLog() += "Incomplete parameters, expect: element, attributeName, attributeValue\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *element = nullptr;
    if (nullptr == element) {
        ScriptRunner::DocumentNode *node = (ScriptRunner::DocumentNode *)JS_GetOpaque(argv[0],
            ScriptRunner::js_nodeClassId);
        if (nullptr != node) {
            element = (ScriptRunner::DocumentElement *)node;
        }
    }
    if (nullptr == element) {
        ScriptRunner::DocumentPart *part = (ScriptRunner::DocumentPart *)JS_GetOpaque(argv[0],
            ScriptRunner::js_partClassId);
        if (nullptr != part) {
            element = (ScriptRunner::DocumentElement *)part;
        }
    }
    if (nullptr == element) {
        ScriptRunner::DocumentComponent *component = (ScriptRunner::DocumentComponent *)JS_GetOpaque(argv[0],
            ScriptRunner::js_componentClassId);
        if (nullptr != component) {
            element = (ScriptRunner::DocumentComponent *)component;
        }
    }
    if (nullptr == element) {
        runner->consoleLog() += "Parameters error\r\n";
        return JS_EXCEPTION;
    }
    const char *attributeName = nullptr;
    const char *attributeValue = nullptr;
    attributeName = JS_ToCString(context, argv[1]);
    if (!attributeName)
        goto fail;
    attributeValue = JS_ToCString(context, argv[2]);
    if (!attributeValue)
        goto fail;
    runner->setAttribute(element, attributeName, attributeValue);
    JS_FreeCString(context, attributeName);
    JS_FreeCString(context, attributeValue);
    return JS_UNDEFINED;
fail:
    JS_FreeCString(context, attributeName);
    JS_FreeCString(context, attributeValue);
    return JS_EXCEPTION;
}

static JSValue js_connect(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 2) {
        runner->consoleLog() += "Incomplete parameters, expect: firstNode, secondNode\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *firstElement = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[0],
        ScriptRunner::js_nodeClassId);
    if (nullptr == firstElement ||
            ScriptRunner::DocumentElementType::Node != firstElement->type) {
        runner->consoleLog() += "First parameter must be node\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *secondElement = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[1],
        ScriptRunner::js_nodeClassId);
    if (nullptr == secondElement ||
            ScriptRunner::DocumentElementType::Node != secondElement->type) {
        runner->consoleLog() += "Second parameter must be node\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentNode *firstNode = (ScriptRunner::DocumentNode *)firstElement;
    ScriptRunner::DocumentNode *secondNode = (ScriptRunner::DocumentNode *)secondElement;
    if (firstNode->part != secondNode->part) {
        runner->consoleLog() += "Cannot connect nodes come from different parts\r\n";
        return JS_EXCEPTION;
    }
    runner->connect(firstNode, secondNode);
    return JS_UNDEFINED;
}

static JSValue js_createComponent(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    ScriptRunner::DocumentComponent *parentComponent = nullptr;
    if (argc >= 1) {
        ScriptRunner::DocumentElement *element = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[0],
            ScriptRunner::js_componentClassId);
        if (nullptr == element ||
                ScriptRunner::DocumentElementType::Component != element->type) {
            runner->consoleLog() += "First parameter must be null or component\r\n";
            return JS_EXCEPTION;
        }
        parentComponent = (ScriptRunner::DocumentComponent *)element;
    }
    JSValue component = JS_NewObjectClass(context, ScriptRunner::js_componentClassId);
    JS_SetOpaque(component, runner->createComponent(parentComponent));
    return component;
}

static JSValue js_createPart(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 1) {
        runner->consoleLog() += "Incomplete parameters, expect: component\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *element = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[0],
        ScriptRunner::js_componentClassId);
    if (nullptr == element ||
            ScriptRunner::DocumentElementType::Component != element->type) {
        runner->consoleLog() += "First parameter must be component\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentComponent *component = (ScriptRunner::DocumentComponent *)element;
    JSValue part = JS_NewObjectClass(context, ScriptRunner::js_partClassId);
    JS_SetOpaque(part, runner->createPart(component));
    return part;
}

static JSValue js_createNode(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 1) {
        runner->consoleLog() += "Incomplete parameters, expect: part\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *element = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[0],
        ScriptRunner::js_partClassId);
    if (nullptr == element ||
            ScriptRunner::DocumentElementType::Part != element->type) {
        runner->consoleLog() += "First parameter must be part\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentPart *part = (ScriptRunner::DocumentPart *)element;
    JSValue node = JS_NewObjectClass(context, ScriptRunner::js_nodeClassId);
    JS_SetOpaque(node, runner->createNode(part));
    return node;
}

static JSValue js_createVariable(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 2) {
        runner->consoleLog() += "Incomplete parameters, expect: name, value\r\n";
        return JS_EXCEPTION;
    }
    
    QString mergedValue;
    
    const char *name = nullptr;
    const char *defaultValue = nullptr;
    
    name = JS_ToCString(context, argv[0]);
    if (!name)
        goto fail;
    defaultValue = JS_ToCString(context, argv[1]);
    if (!defaultValue)
        goto fail;
    
    mergedValue = runner->createVariable(name, defaultValue);
    JS_FreeCString(context, name);
    JS_FreeCString(context, defaultValue);
    
    return JS_NewString(context, mergedValue.toUtf8().constData());
    
fail:
    JS_FreeCString(context, name);
    JS_FreeCString(context, defaultValue);
    return JS_EXCEPTION;
}

ScriptRunner::ScriptRunner()
{
}

ScriptRunner::~ScriptRunner()
{
    delete m_resultSnapshot;
    delete m_defaultVariables;
    for (auto &it: m_parts)
        delete it;
    for (auto &it: m_components)
        delete it;
    for (auto &it: m_nodes)
        delete it;
}

QString &ScriptRunner::consoleLog()
{
    return m_consoleLog;
}

const QString &ScriptRunner::scriptError()
{
    return m_scriptError;
}

ScriptRunner::DocumentPart *ScriptRunner::createPart(DocumentComponent *component)
{
    ScriptRunner::DocumentPart *part = new ScriptRunner::DocumentPart;
    part->component = component;
    m_parts.push_back(part);
    return part;
}

ScriptRunner::DocumentComponent *ScriptRunner::createComponent(DocumentComponent *parentComponent)
{
    ScriptRunner::DocumentComponent *component = new ScriptRunner::DocumentComponent;
    component->parentComponent = parentComponent;
    m_components.push_back(component);
    return component;
}

ScriptRunner::DocumentNode *ScriptRunner::createNode(DocumentPart *part)
{
    ScriptRunner::DocumentNode *node = new ScriptRunner::DocumentNode;
    node->part = part;
    m_nodes.push_back(node);
    return node;
}

bool ScriptRunner::setAttribute(DocumentElement *element, const QString &name, const QString &value)
{
    element->attributes[name] = value;
    
    // TODO: Validate attribute name and value
    
    return true;
}

void ScriptRunner::connect(DocumentNode *firstNode, DocumentNode *secondNode)
{
    m_edges.push_back(std::make_pair(firstNode, secondNode));
}

static void js_componentFinalizer(JSRuntime *runtime, JSValue value)
{
    ScriptRunner::DocumentComponent *component = (ScriptRunner::DocumentComponent *)JS_GetOpaque(value,
        ScriptRunner::js_componentClassId);
    if (component) {
        component->deleted = true;
    }
}

static JSClassDef js_componentClass = {
    "Component",
    .finalizer = js_componentFinalizer,
};

static void js_nodeFinalizer(JSRuntime *runtime, JSValue value)
{
    ScriptRunner::DocumentNode *node = (ScriptRunner::DocumentNode *)JS_GetOpaque(value,
        ScriptRunner::js_nodeClassId);
    if (node) {
        node->deleted = true;
    }
}

static JSClassDef js_nodeClass = {
    "Node",
    .finalizer = js_nodeFinalizer,
};

static void js_partFinalizer(JSRuntime *runtime, JSValue value)
{
    ScriptRunner::DocumentPart *part = (ScriptRunner::DocumentPart *)JS_GetOpaque(value,
        ScriptRunner::js_partClassId);
    if (part) {
        part->deleted = true;
    }
}

static JSClassDef js_partClass = {
    "Part",
    .finalizer = js_partFinalizer,
};

void ScriptRunner::run()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    // Warning: Not thread safe, but we have only one script instance running, so it doesn't matter
    js_partClassId = JS_NewClassID(&js_partClassId);
    js_componentClassId = JS_NewClassID(&js_componentClassId);
    js_nodeClassId = JS_NewClassID(&js_nodeClassId);
    
    m_defaultVariables = new std::map<QString, std::map<QString, QString>>;

    JSRuntime *runtime = JS_NewRuntime();
    JSContext *context = JS_NewContext(runtime);
    
    JS_NewClass(runtime, js_partClassId, &js_partClass);
    JS_NewClass(runtime, js_componentClassId, &js_componentClass);
    JS_NewClass(runtime, js_nodeClassId, &js_nodeClass);
    
    JS_SetContextOpaque(context, this);
    
    if (nullptr != m_script &&
            !m_script->isEmpty()) {
        auto buffer = m_script->toUtf8();
        
        JSValue globalObject = JS_GetGlobalObject(context);
        
        JSValue document = JS_NewObject(context);
        JS_SetPropertyStr(context,
            document, "createComponent",
            JS_NewCFunction(context, js_createComponent, "createComponent", 1));
        JS_SetPropertyStr(context,
            document, "createPart",
            JS_NewCFunction(context, js_createPart, "createPart", 1));
        JS_SetPropertyStr(context,
            document, "createNode",
            JS_NewCFunction(context, js_createNode, "createNode", 1));
        JS_SetPropertyStr(context,
            document, "createVariable",
            JS_NewCFunction(context, js_createVariable, "createVariable", 2));
        JS_SetPropertyStr(context,
            document, "connect",
            JS_NewCFunction(context, js_connect, "connect", 2));
        JS_SetPropertyStr(context,
            document, "setAttribute",
            JS_NewCFunction(context, js_setAttribute, "setAttribute", 3));
        JS_SetPropertyStr(context, globalObject, "document", document);
        
        JSValue console = JS_NewObject(context);
        JS_SetPropertyStr(context,
            console, "log",
            JS_NewCFunction(context, js_print, "log", 1));
        JS_SetPropertyStr(context, globalObject, "console", console);
        
        JS_FreeValue(context, globalObject);
        
        JSValue object = JS_Eval(context, buffer.constData(), buffer.size(), "",
            JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(object)) {
            JSValue exceptionValue = JS_GetException(context);
            bool isError = JS_IsError(context, exceptionValue);
            if (isError) {
                m_scriptError += "Throw: ";
                const char *exceptionError = JS_ToCString(context, exceptionValue);
                m_scriptError += exceptionError;
                m_scriptError += "\r\n";
                JS_FreeCString(context, exceptionError);
                JSValue value = JS_GetPropertyStr(context, exceptionValue, "stack");
                if (!JS_IsUndefined(value)) {
                    const char *stack = JS_ToCString(context, value);
                    m_scriptError += stack;
                    m_scriptError += "\r\n";
                    JS_FreeCString(context, stack);
                }
                JS_FreeValue(context, value);
                
                qDebug() << "QuickJS" << m_scriptError;
            }
        } else {
            generateSnapshot();
            const char *objectString = JS_ToCString(context, object);
            qDebug() << "Result:" << objectString;
            JS_FreeCString(context, objectString);
        }
        
        JS_FreeValue(context, object);
    }
    
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    
    qDebug() << "The script run" << countTimeConsumed.elapsed() << "milliseconds";
}

void ScriptRunner::generateSnapshot()
{
    m_resultSnapshot = new Snapshot;
    std::map<void *, QString> pointerToIdMap;
    
    std::map<void *, QStringList> componentChildrensMap;
    
    QStringList rootChildren;
    
    for (const auto &it: m_components) {
        QString idString = QUuid::createUuid().toString();
        pointerToIdMap[it] = idString;
        auto &component = m_resultSnapshot->components[idString];
        component = it->attributes;
        component["id"] = idString;
        componentChildrensMap[it->parentComponent].append(idString);
        if (nullptr == it->parentComponent)
            rootChildren.append(idString);
    }
    m_resultSnapshot->rootComponent["children"] = rootChildren.join(",");
    
    for (const auto &it: m_components) {
        const auto &idString = pointerToIdMap[it];
        auto &component = m_resultSnapshot->components[idString];
        auto &childrens = componentChildrensMap[it];
        if (!childrens.empty())
            component["children"] = childrens.join(",");
    }
    
    for (const auto &it: m_parts) {
        auto findComponent = pointerToIdMap.find(it->component);
        if (findComponent == pointerToIdMap.end()) {
            m_scriptError += "Find component pointer failed, component maybe deleted\r\n";
            continue;
        }
        
        QString idString = QUuid::createUuid().toString();
        pointerToIdMap[it] = idString;
        auto &part = m_resultSnapshot->parts[idString];
        part = it->attributes;
        part.insert({"visible", "true"});
        part["id"] = idString;
        
        auto &component = m_resultSnapshot->components[findComponent->second];
        component["linkData"] = idString;
        component["linkDataType"] = "partId";
    }
    for (const auto &it: m_nodes) {
        auto findPart = pointerToIdMap.find(it->part);
        if (findPart == pointerToIdMap.end()) {
            m_scriptError += "Find part pointer failed, part maybe deleted\r\n";
            continue;
        }
        QString idString = QUuid::createUuid().toString();
        pointerToIdMap[it] = idString;
        auto &node = m_resultSnapshot->nodes[idString];
        node = it->attributes;
        node["id"] = idString;
        node["partId"] = findPart->second;
    }
    for (const auto &it: m_edges) {
        if (it.first->part != it.second->part) {
            m_scriptError += "Cannot connect nodes come from different parts\r\n";
            continue;
        }
        auto findFirstNode = pointerToIdMap.find(it.first);
        if (findFirstNode == pointerToIdMap.end()) {
            m_scriptError += "Find first node pointer failed, node maybe deleted\r\n";
            continue;
        }
        auto findSecondNode = pointerToIdMap.find(it.second);
        if (findSecondNode == pointerToIdMap.end()) {
            m_scriptError += "Find second node pointer failed, node maybe deleted\r\n";
            continue;
        }
        auto findPart = pointerToIdMap.find(it.first->part);
        if (findPart == pointerToIdMap.end()) {
            m_scriptError += "Find part pointer failed, part maybe deleted\r\n";
            continue;
        }
        QString idString = QUuid::createUuid().toString();
        auto &edge = m_resultSnapshot->edges[idString];
        edge["id"] = idString;
        edge["from"] = findFirstNode->second;
        edge["to"] = findSecondNode->second;
        edge["partId"] = findPart->second;
    }
    
    for (const auto &it: m_resultSnapshot->nodes) {
        qDebug() << "Generated node:" << it.second;
    }
    for (const auto &it: m_resultSnapshot->edges) {
        qDebug() << "Generated edge:" << it.second;
    }
    for (const auto &it: m_resultSnapshot->parts) {
        qDebug() << "Generated part:" << it.second;
    }
    for (const auto &it: m_resultSnapshot->components) {
        qDebug() << "Generated component:" << it.second;
    }
}

QString ScriptRunner::createVariable(const QString &name, const QString &defaultValue)
{
    if (nullptr != m_defaultVariables) {
        if (m_defaultVariables->find(name) != m_defaultVariables->end()) {
            m_scriptError += "Repeated variable name found: \"" + name + "\"";
        }
        (*m_defaultVariables)[name]["value"] = defaultValue;
    }
    if (nullptr != m_variables) {
        auto findVariable = m_variables->find(name);
        if (findVariable != m_variables->end()) {
            auto findValue = findVariable->second.find("value");
            if (findValue != findVariable->second.end())
                return findValue->second;
        }
    }
    return defaultValue;
}

void ScriptRunner::process()
{
    run();
    emit finished();
}

void ScriptRunner::setScript(QString *script)
{
    m_script = script;
}

void ScriptRunner::setVariables(std::map<QString, std::map<QString, QString>> *variables)
{
    m_variables = variables;
}

Snapshot *ScriptRunner::takeResultSnapshot()
{
    Snapshot *snapshot = m_resultSnapshot;
    m_resultSnapshot = nullptr;
    return snapshot;
}

std::map<QString, std::map<QString, QString>> *ScriptRunner::takeDefaultVariables()
{
    std::map<QString, std::map<QString, QString>> *defaultVariables = m_defaultVariables;
    m_defaultVariables = nullptr;
    return defaultVariables;
}

void ScriptRunner::mergeVaraibles(std::map<QString, std::map<QString, QString>> *target, const std::map<QString, std::map<QString, QString>> &source)
{
    for (const auto &it: source) {
        (*target)[it.first] = it.second;
    }
}
