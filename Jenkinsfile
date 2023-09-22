pipeline {
  agent any
  stages {
    stage('build') {
      parallel {
        stage('build') {
          steps {
            echo 'Build Step'
            isUnix()
            sh '''ls
make random'''
          }
        }

        stage('Run') {
          steps {
            sh 'python execution.py'
          }
        }

      }
    }

  }
}