import groovy.json.JsonSlurperClassic

node {
    def ARTIFACTS_PATH0 = 'build/repository/org/muhkuh/lua/romloader/**'
    def ARTIFACTS_PATH1 = 'targets/jonchki/repository/org/muhkuh/lua/katscha/**'
    def strBuilds = env.JENKINS_SELECT_BUILDS
    def atBuilds = new JsonSlurperClassic().parseText(strBuilds)

    atBuilds.each { atEntry ->
        stage("${atEntry[0]} ${atEntry[1]} ${atEntry[2]}"){
            docker.image("${atEntry[3]}").inside('-u root') {
                /* Clean before the build. */
                sh 'rm -rf .[^.] .??* *'

                checkout([$class: 'GitSCM',
                    branches: [[name: '*/master']],
                    doGenerateSubmoduleConfigurations: false,
                    extensions: [
                        [$class: 'SubmoduleOption',
                            disableSubmodules: false,
                            recursiveSubmodules: true,
                            reference: '',
                            trackingSubmodules: false
                        ]
                    ],
                    submoduleCfg: [],
                    userRemoteConfigs: [[url: 'https://github.com/muhkuh-sys/org.muhkuh.lua-katscha.git']]
                ])

                /* Build the project. */
                sh "python2.7 build_artifact.py ${atEntry[0]} ${atEntry[1]} ${atEntry[2]}"

                /* Archive all artifacts. */
                archiveArtifacts artifacts: "${ARTIFACTS_PATH0}/*.tar.xz,${ARTIFACTS_PATH0}/*.xml,${ARTIFACTS_PATH0}/*.hash,${ARTIFACTS_PATH0}/*.pom,${ARTIFACTS_PATH1}/*.zip"

                /* Clean up after the build. */
                sh 'rm -rf .[^.] .??* *'
            }
        }
    }
}
