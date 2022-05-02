function TestFunction( argsTab )
  retVal = {}
  retVal["str"] = "String: " .. argsTab["string"]
  retVal["int"] = "Integer: " .. argsTab["integer"]
  retVal["float"] = "Float: " .. argsTab["float"]
  return retVal
end

