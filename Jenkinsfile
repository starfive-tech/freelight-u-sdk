pipeline {
    agent { label 'amd64'}

    stages {
        stage('Build') {
            steps {
                sh '/bin/bash ./build.sh'
            }
        }
    }
}
