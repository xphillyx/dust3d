#include <QDebug>
#include <QElapsedTimer>
#include "scriptrunner.h"
extern "C" {
#include "quickjs.h"
}

static JSValue js_createComponent(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    qDebug() << "jsCreateComponent";
    JSValue component = JS_NewObject(context);
    return component;
}

static JSValue js_createPart(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    qDebug() << "jsCreatePart";
    JSValue part = JS_NewObject(context);
    return part;
}

static JSValue js_createNode(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    qDebug() << "jsCreateNode";
    JSValue node = JS_NewObject(context);
    return node;
}

static JSValue js_createVariable(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    
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
}

const QString &ScriptRunner::scriptError()
{
    return m_scriptError;
}

void ScriptRunner::run()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    m_defaultVariables = new std::map<QString, std::map<QString, QString>>;

    JSRuntime *runtime = JS_NewRuntime();
    JSContext *context = JS_NewContext(runtime);
    
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
        
        JS_SetPropertyStr(context, globalObject, "document", document);
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
