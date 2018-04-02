
templateParamDef = 'template<> typename SensingTypeInfo<SEN_T>::ValType ReflectiveEngine::predict<SEN_T,ResourceT>(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc);'

templateNoParamDef = 'template<> typename SensingTypeInfo<SEN_T>::ValType ReflectiveEngine::predict<SEN_T,ResourceT>(const ResourceT *rsc);'

templateParam = """
template<>
typename SensingTypeInfo<SEN_T>::ValType
ReflectiveEngine::predict<SEN_T,ResourceT>(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}"""

templateNoParam = """
template<>
typename SensingTypeInfo<SEN_T>::ValType
ReflectiveEngine::predict<SEN_T,ResourceT>(const ResourceT *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}"""


senTypesParam = [
    'SEN_PERFCNT',
    'SEN_BEATS'
]
senTypesNoParam = [
    'SEN_TOTALTIME_S',
    'SEN_BUSYTIME_S',
    'SEN_NIVCSW',
    'SEN_NVCSW',
    'SEN_FREQ_MHZ',
    'SEN_LASTCPU',
    'SEN_POWER_W',
    'SEN_TEMP_C'
]

rscTypes = [
    'tracked_task_data_t',
    'core_info_t',
    'freq_domain_info_t',
    'power_domain_info_t'
]

print('///////////////////////////////////////////////////////////////////')
print('// Specialization declarations.')
print('// Needed so we dont care about the order implementations appear')
print()
for rsc in rscTypes:
    for sen in senTypesParam:
        print(templateParamDef.replace('SEN_T', sen).replace('ResourceT', rsc))
    for sen in senTypesNoParam:
        print(templateNoParamDef.replace('SEN_T', sen).replace('ResourceT', rsc))
print('\n///////////////////////////////////////////////////////////////////')


for rsc in rscTypes:
    print('\n\n/////////////////////////////////////////////////')
    print('// For resource type \'{}\''.format(rsc))
    print('/////////////////////////////////////////////////')
    for sen in senTypesParam:
        print(templateParam.replace('SEN_T', sen).replace('ResourceT', rsc))
    for sen in senTypesNoParam:
        print(templateNoParam.replace('SEN_T', sen).replace('ResourceT', rsc))
