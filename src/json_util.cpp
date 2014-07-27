/**
 * Copyright 2014, Planet Labs, Inc.
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

#include "compositor.h"

/************************************************************************/
/*                            PLParseJson()                            */
/************************************************************************/

json_object *PLParseJson(const char *json_string)

{
    json_tokener* jstok = NULL;
    json_object* jsobj = NULL;

    jstok = json_tokener_new();
    jsobj = json_tokener_parse_ex(jstok, json_string, -1);
    if( jstok->err != json_tokener_success)
    {
        CPLError( CE_Warning, CPLE_AppDefined,
                  "JSON parsing failure: %s (at offset %d)",
                  json_tokener_errors[jstok->err], jstok->char_offset);
        
        json_tokener_free(jstok);
        return NULL;
    }
    else
    {
        json_tokener_free(jstok);

        return jsobj;
    }
}

/************************************************************************/
/*                          PLFindJSONChild()                          */
/************************************************************************/

json_object *PLFindJSONChild(json_object *json, const char *path, 
                              int create)
{
    if( path == NULL || strlen(path) == 0)
        return json;

    CPLStringList path_items(CSLTokenizeStringComplex(path, ".", FALSE, FALSE));
    int i;
    for(i=0; i < path_items.size(); i++)
    {
        json_object *parent = json;

        if(!json_object_is_type(parent, json_type_object))
            return NULL;

        json = json_object_object_get(parent, path_items[i]);
        if(json == NULL)
        {
            if(!create)
                return NULL;
            json = json_object_new_object();
            json_object_object_add(parent, path_items[i], json);
        }
    }

    return json;
}

/************************************************************************/
/*                          PLGetJSONString()                          */
/************************************************************************/

CPLString PLGetJSONString(json_object *json, const char *path, 
                           const char *default_value)
{
    json_object *obj = PLFindJSONChild(json, path, FALSE);
    if( obj == NULL)
    {
        if( default_value == NULL )
            return "NULL";
        else
            return default_value;
    }
    else
    {
        return json_object_get_string(obj);
    }        
}

/************************************************************************/
/*                         PLValidateJSONNode()                         */
/*                                                                      */
/*      Should be something like:                                       */
/*        parm_name:[o,r]:[string,number,array,object],...              */
/*                                                                      */
/*      eg.                                                             */
/*        "output_file:r:string,quality_file:o:string"                  */
/************************************************************************/

void PLValidateJSONNode(json_object *node, 
                        const char *definition)

{
    CPLStringList parm_defs(CSLTokenizeString2(definition,",",0));
    std::map<CPLString,CPLString> parmOptions;
    std::map<CPLString,CPLString> parmTypes;
    
    for( int iParm=0; iParm < parm_defs.size(); iParm++ )
    {
        CPLStringList parts(CSLTokenizeString2(parm_defs[iParm], ":", 0));

        CPLAssert(parts.size() == 3);
        
        parmOptions[parts[0]] = parts[1];
        parmOptions[parts[0]] = parts[2];

        
    }
                            
}

