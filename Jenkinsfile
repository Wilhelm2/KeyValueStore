pipeline {
  agent any
  stages {
    stage('build') {
      steps {
        echo 'Build Step'
        isUnix()
        sh '''ls
make random'''
      }
    }

  }
}