<%
import re
namespace = data['rootNamespace'] + '.Internal'
property_id_type = namespace + '.' + data['propertyIdClassName']
header_file = data['headerP4Path']
max_buffer_size = 256
dll_name = data['dll']
class_name = data['pInvokeClassName']
pinvoke_file_name = data['pInvokeClassName']
array_brackets_expression = re.compile(r'\[.*\]')

RESERVED_WORDS = [
    'abstract', 'as',
    'base', 'bool', 'break','byte',
    'case', 'catch', 'char', 'checked', 'class', 'const', 'continue',
    'decimal', 'default', 'delegate', 'do', 'double',
    'else', 'enum', 'event', 'explicit', 'extern',
    'false', 'finally', 'fixed', 'float', 'for', 'foreach',
    'goto',
    'if', 'implicit', 'in', 'int', 'interface', 'internal', 'is',
    'lock', 'long',
    'namespace', 'new', 'null',
    'object', 'operator', 'out', 'override',
    'params', 'private', 'protected', 'public',
    'readonly', 'ref', 'return',
    'sbyte', 'sealed', 'short', 'sizeof', 'stackalloc', 'static', 'status', 'string', 'struct', 'switch',
    'this', 'throw', 'true', 'try', 'typeof',
    'uint', 'ulong', 'unchecked', 'unsafe', 'ushort', 'using',
    'virtual', 'void', 'volatile',
    'while'
]

def sanitize_cnames(parameters):
    """Sanitizes cname fields on a list of parameter objects and populates the dotNetName field with the sanitized value."""
    for parameter in parameters:
        if parameter['cname'] in RESERVED_WORDS:
            parameter['dotNetName'] = parameter['cname'] + 'Parameter'

def get_proto_type(typestr):
    if 'ViBoolean' in typestr:
        return 'bool'
    if 'ViReal64' in typestr:
        return 'double'
    if 'ViInt32' in typestr:
        return 'int32'
    if 'ViConstString' in typestr:
        return 'string'
    if 'ViString' in typestr:
        return 'string'
    if 'ViRsrc' in typestr:
        return 'string'
    if 'ViChar' in typestr:
        return 'string'
    if 'niScope_wfmInfo' in typestr:
        return 'ScopeWaveformInfo'
    if 'niScope_coefficientInfo' in typestr:
        return 'ScopeCoefficientInfo'
    if 'ViReal32' in typestr:
        return 'float'
    if 'ViAttr' in typestr:
        return 'ScopePropertyId'
    if 'ViInt8' in typestr:
        return 'bytes'
    if 'void' in typestr:
        return 'bytes'
    if 'ViInt16' in typestr:
        return 'bytes'
    if 'ViInt64' in typestr:
        return 'int64'
    if 'ViUInt32' in typestr:
        return 'uint32'
    if 'ViUInt64' in typestr:
        return 'uint64'
    if 'ViStatus' in typestr:
        return 'int32'
    if 'ViAddr' in typestr:
        return 'bytes'
    if 'NIComplexNumber' in typestr:
        return 'ComplexNumber'
    if 'NIComplexI16' in typestr:
        return 'ComplexI16'
    if 'int' == typestr:
        return 'int32'
    return typestr

def is_output_parameter(parameter):
    if 'numberOf' in parameter['cname']:
        return False
    if 'out ' in parameter['nettype']:
        return True
    if 'Out ' in parameter['nettype']:
        return True
    if '[Out]' in parameter['nettype']:
        return True
    if 'bufferSize' in parameter['cname']:
        return False
    if 'rpcdirection' in parameter:
        if 'out' in parameter['rpcdirection']:
            return True
    return False

def is_input_parameter(parameter):
    if is_output_parameter(parameter) == True:
        return False
    if 'numberOf' in parameter['cname']:
        return False
    if 'bufferSize' in parameter['cname']:
        return False
    return True

def get_repeated_prefix(parameter):
    if '[]' in parameter['nettype']:
        return 'repeated '
    return ''

%>\
//---------------------------------------------------------------------
//---------------------------------------------------------------------
syntax = "proto3";

//---------------------------------------------------------------------
//---------------------------------------------------------------------
option java_multiple_files = true;
option java_package = "ni.niScope";
option java_outer_classname = "niScope";
option objc_class_prefix = "NI";

//---------------------------------------------------------------------
//---------------------------------------------------------------------
package niScope;

//---------------------------------------------------------------------
// The niScope service definition.
//---------------------------------------------------------------------
service niScope {
% for function in data['functions']:
<%
if 'generate' in function:
    if function['generate'] != true:
        continue;

function_name = function['name'][8:]
method_name = function['netname']
index = 1
%>\
    rpc ${method_name}(${method_name}Parameters) returns (${method_name}Result) {};
% endfor
}

message ViSession {
  uint32 id = 1;
}

message ScopeWaveformInfo {
  double absoluteInitialX = 1;
  double relativeInitialX = 2;
  double xIncrement = 3;
  int32 actualNumberOfSamples = 4;
  double offset = 5;
  double gain = 6;
}

message ScopeCoefficientInfo {
  double offset = 1;
  double gain = 2;
}

message ScopePropertyId {
  int32 propertyId = 1;
}

message ComplexNumber {
    double real = 1;
    double imaginary = 2;
}

message ComplexI16 {
    bytes real = 1;
    bytes imaginary = 2;
}

% for function in data['functions']:
<%
if 'generate' in function:
    if function['generate'] != true:
        continue;

method_name = function['netname']
parameters = function['parameters']
sanitize_cnames(parameters)
%>\
message ${method_name}Parameters {
<%
index = 0
%>\
%for parameter in parameters:
<%
    if is_input_parameter(parameter) == False:
        continue;
    index = index + 1
%>\
  ${get_repeated_prefix(parameter)}${get_proto_type(parameter['ctype'])} ${get_proto_type(parameter['cname'])} = ${index};
%endfor
}
    
message ${method_name}Result {
  int32 status = 1;
<%
index = 1
%>\
%for parameter in parameters:
<%
    if is_output_parameter(parameter) == False:
        continue;
    index = index + 1
%>\
  ${get_repeated_prefix(parameter)}${get_proto_type(parameter['ctype'])} ${get_proto_type(parameter['cname'])} = ${index};
%endfor
}

% endfor
