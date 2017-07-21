def unitTestArguments = "-s -c SMemFunctionalTests -e PRIMS_Sanity1 -f testSmemArithmetic"
def unitTestArgumentsWindows = "-s -c SMemFunctionalTests -e testCommandToFile -e testHamilton -e testSmemArithmetic -e PRIMS_Sanity1"

def names = nodeNames()
def builders = [:]

for (int i=0; i<names.size(); ++i) {
  def name = names[i]

  builders["node_" + name] = {
    node(name) {
      checkout scm
      
      stage('Clean')
      {
        if (isUnix()) {
          sh 'rm -f *.7zip'
          sh 'rm -rf out*'
          sh 'rm -rf build/Core/ClientSMLSWIG*'

        } else {
          bat 'del /q /f *.7zip'
          bat 'del /q /f user-env*.bat'
          bat 'if exist "build\\Core\\ClientSMLSWIG" del /q /f build\\Core\\ClientSMLSWIG*'
          bat 'if exist "out" del /q /f out'
        }
      }
      stage('Build')
      {
          if (isUnix()) {
            sh 'scons all --scu'
          } else {
            def tcl="C:\\Tcl"
            if (name == "Windows32") {
              tcl="C:\\Tcl"
              bat 'echo set PYTHON_HOME=C:\\Python27>> user-env.bat'
            } else {
              tcl="C:\\Tcl-x86-64"
              bat 'echo set PYTHON_HOME=C:\\Python27-64>> user-env.bat'
            }
    
            bat 'echo set JAVA_HOME=C:\\Program Files\\Java\\jdk1.7.0_79>> user-env.bat'
            bat 'echo set SWIG_HOME=C:\\swigwin\\>> user-env.bat'
    
            bat "%VS_2015% & call build.bat all --scu --tcl=" + tcl
          }
      }
      stage('Upload')
      {
          withCredentials([[$class: 'UsernamePasswordMultiBinding', credentialsId: '099da30c-b551-4c0c-847d-28fa1c22c5cb',
                                usernameVariable: 'USERNAME', passwordVariable: 'PASSWORD']]) {
            if (isUnix()) {
              sh "export VERSION=\$(<soarversion); 7za a \${VERSION}-" + name + ".7zip out/"
              sh "export VERSION=\$(<soarversion); sshpass -p \${PASSWORD} scp \${VERSION}-" + name + ".7zip \${USERNAME}@soar-jenkins.eecs.umich.edu:/Users/Shared/Build/Nightlies/"
            } else {
              bat 'for /f %%x in (soarversion) do "C:/Program Files/7-Zip/7z.exe" a %%x-' + name + '.7zip out/'
              bat 'for /f %%x in (soarversion) do C:\\pscp.exe -pw %PASSWORD% %%x-' + name + '.7zip %USERNAME%@soar-jenkins.eecs.umich.edu:/Users/Shared/Build/Nightlies/'
            }
          }
      }
      stage('Test')
      {
          if (isUnix()) {
            sh 'pushd out; ./UnitTests '  + unitTestArguments + '; popd'
            junit 'out/TestResults.xml'
          } else {
            bat 'out\\UnitTests.exe ' + unitTestArguments
            //bat 'pushd out & UnitTests ' + unitTestArguments + ' & popd'
            junit 'TestResults.xml'
          }
      }
      archive '*.7zip'
    }
  }
}

parallel builders

@NonCPS
def nodeNames() {
  return jenkins.model.Jenkins.instance.nodes.collect { node -> node.name }
}
