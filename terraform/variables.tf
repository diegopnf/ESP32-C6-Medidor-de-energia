variable "aws_region" {
  description = "Região da AWS para o deploy"
  type        = string
  default     = "us-east-1"
}

variable "project_name" {
  description = "Nome do projeto para identificação de recursos"
  type        = string
  default     = "dashboard-de-energia-eletrica"
}

variable "device_id" {
  description = "ID único do dispositivo para o teste"
  type        = string
  default     = "medidor-esp32c6-01"
}