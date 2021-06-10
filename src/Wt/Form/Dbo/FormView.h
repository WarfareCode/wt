// This may look like C code, but it's really -*- C++ -*-
/*
 * Copyright (C) 2021 Emweb bv, Herent, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#ifndef WT_FORM_DBO_FORMVIEW_H_
#define WT_FORM_DBO_FORMVIEW_H_

#include <Wt/WTemplateFormView.h>
#include <Wt/Form/WAbstractFormDelegate.h>
#include <Wt/Form/Dbo/Actions.h>
#include <Wt/Form/Dbo/FormModel.h>

namespace Wt {
  namespace Form {
    namespace Dbo {
/*! \brief A view class to represent database objects
 */
template<class C>
class FormView : public Wt::WTemplateFormView
{
public:
  explicit FormView(const Wt::WString& text)
    : Wt::WTemplateFormView(text)
  {
  }

  /*! \brief Sets the form model
   *
   * This method will automatically generate the form delegates
   * and set the form widgets and model validators.
   */
  void setFormModel(std::shared_ptr<FormModel<C>> model)
  {
    model_ = model;

    C dummy;

    // Automatically generate the form delegates
    ViewAction action(model->session(), model.get(), formDelegates_);
    dummy.persist(action);

    for (const Wt::WFormModel::Field& f : model->fields()) {
      setFormWidget(f, formWidget(f));
      model->setValidator(f, validator(f));
    }

    updateView(model.get());
  }

  /*! \brief Sets a custom form delegate
   *
   * Overrides the default delegate for a given field
   *
   * This method will throw an exception if it's called after setFormModel.
   */
  void setFormDelegate(Wt::WFormModel::Field field, std::shared_ptr<Wt::Form::WAbstractFormDelegate> delegate)
  {
    if (model_) {
      throw Wt::WException("Form Delegates cannot be set after the model has been initialized!");
    }

    if (!delegate) {
      // Erase from map
      auto it = formDelegates_.find(field);
      if (it != formDelegates_.end()) {
        formDelegates_.erase(it);
      }
    } else {
      formDelegates_[field] = std::move(delegate);
    }
  }

  /*! \brief Updates a value in the Model
   */
  void updateModelValue(Wt::WFormModel *model, Wt::WFormModel::Field field, Wt::WFormWidget *edit) override
  {
    if (updateModelValue(model, field, static_cast<Wt::WWidget*>(edit))) {
      return;
    }

    std::shared_ptr<Wt::Form::WAbstractFormDelegate> d = delegate(field);
    if (d) {
      d->updateModelValue(model, field, edit);
    } else {
      Wt::WTemplateFormView::updateModelValue(model, field, edit);
    }
  }

  /*! \brief Updates a value in the Model
   */
  bool updateModelValue(Wt::WFormModel *model, Wt::WFormModel::Field field, Wt::WWidget *edit) override
  {
    std::shared_ptr<Wt::Form::WAbstractFormDelegate> d = delegate(field);
    if (d) {
      return d->updateModelValue(model, field, edit);
    } else {
      return Wt::WTemplateFormView::updateModelValue(model, field, edit);
    }
  }

  /*! \brief Updates a value in the View
   */
  void updateViewValue(Wt::WFormModel *model, Wt::WFormModel::Field field, Wt::WFormWidget *edit) override
  {
    if (updateViewValue(model, field, static_cast<Wt::WWidget*>(edit))) {
      return;
    }

    std::shared_ptr<Wt::Form::WAbstractFormDelegate> d = delegate(field);
    if (d) {
      d->updateViewValue(model, field, edit);
    } else {
      Wt::WTemplateFormView::updateViewValue(model, field, edit);
    }
  }

  /*! \brief Updates a value in the View
   */
  bool updateViewValue(Wt::WFormModel *model, Wt::WFormModel::Field field, Wt::WWidget *edit) override
  {
    std::shared_ptr<Wt::Form::WAbstractFormDelegate> d = delegate(field);
    if (d) {
      return d->updateViewValue(model, field, edit);
    } else {
      return Wt::WTemplateFormView::updateViewValue(model, field, edit);
    }
  }

  void save()
  {
    updateModel(model_.get());
    if (model_->validate()) {
      model_->saveDboValues();
      updateView(model_.get());
      saved().emit();
    } else {
      // update the view to show the validation text
      updateView(model_.get());
      validationFailed().emit();
    }
  }

protected:
  /*! \brief Customize the auto generate form widget
   *
   * Allows derived classes to customize the automatically generated widget
   * without having to customize an entire WFormDelegate.
   *
   * Base class implementation doesn't modify the widget
   */
  virtual void customizeFormWidget(Wt::WFormModel::Field field, Wt::WWidget *widget)
  {
  }

  /*! \brief Customize the auto generated validator
   *
   * Allows derived classes to customize the automatically generated validator
   * without having to customize an entire WFormDelegate.
   *
   * For example: the default validator for integers is a WIntValidator. This
   * method allows a derived class to specify the range for the validator.
   *
   * Base class implementation doesn't modify the validator
   */
  virtual void customizeValidator(Wt::WFormModel::Field field, Wt::WValidator *validator)
  {
  }

  /*! \brief Signal emitted when form is saved
   */
  Wt::Signal<>& saved() { return saved_; }

  /*! \brief Signal emitted when validation failed
   *
   * This can be emitted when saving the form. The save action
   * will have failed because some fields are invalid.
   */
  Wt::Signal<>& validationFailed() { return validationFailed_; }

private:
  std::shared_ptr<FormModel<C>> model_;
  std::map<std::string, std::shared_ptr<Wt::Form::WAbstractFormDelegate>> formDelegates_;

  Wt::Signal<> saved_;
  Wt::Signal<> validationFailed_;

  /*! \brief Gets the widget generated by the form delegate
   */
  std::unique_ptr<Wt::WWidget> formWidget(Wt::WFormModel::Field field)
  {
    std::shared_ptr<Wt::Form::WAbstractFormDelegate> d = delegate(field);
    if (d) {
      std::unique_ptr<Wt::WWidget> widget = d->createFormWidget();
      customizeFormWidget(field, widget.get());
      return widget;
    }
    return nullptr;
  }

  /*! \brief Gets the validator generated by the form delegate
   */
  std::shared_ptr<Wt::WValidator> validator(Wt::WFormModel::Field field)
  {
    std::shared_ptr<Wt::Form::WAbstractFormDelegate> d = delegate(field);
    if (d) {
      std::shared_ptr<Wt::WValidator> validator = d->createValidator();
      customizeValidator(field, validator.get());
      return validator;
    }
    return nullptr;
  }

  /*! \brief Gets the form deleate
   */
  std::shared_ptr<Wt::Form::WAbstractFormDelegate> delegate(Wt::WFormModel::Field field)
  {
    auto it = formDelegates_.find(field);
    if (it != formDelegates_.end()) {
      return it->second;
    }
    return nullptr;
  }
};
    }
  }
}

#endif // WT_FORM_DBO_FORMVIEW