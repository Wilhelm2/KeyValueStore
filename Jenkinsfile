pipeline {
    agent any
    stages {
        stage('build') {
            parallel {
                stage('build') {
                    steps {
                        echo 'Build Step'
                        sh 'make random'
                    }
                }
            }
        }

        stage('Run') {
            steps {
                when{ isUnix() }
                    sh 'python execution.py'
            }
        }

        stage('end') {
            steps {
                echo 'end'
            }
        }

    }
}
